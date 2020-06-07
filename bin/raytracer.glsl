#version 430

layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba32f, binding=0) uniform image2D out_Color;

#define MAX_SPHERE_SIZE 20
#define MAX_LIGHT_SIZE 8

#define INFINITY (1e+300 * 1e+300)

struct Ray {
    vec3 position;
    vec3 direction;
};

struct Camera {
    vec3 position;
    vec3 forward;
    vec3 right;
    vec3 up;
    float width;
    float height;
};

uniform Camera camera;

struct Sphere {
    vec3 position;
    float radius;
    vec3 color;
    // 0.0 = rougher, 1.0 = smoother [ specularFactor * 256.0f]
    float specularFactor;
};

uniform Sphere spheres[MAX_SPHERE_SIZE];
uniform int sphereCount;

struct Light {
    vec3 position;
    float intensity;
    vec3 color;
};

uniform Light lights[MAX_LIGHT_SIZE];
uniform int lightCount;


vec2 intersection(in Ray ray, in Sphere sphere) {
    vec3 v = ray.position - sphere.position;

    float k1 = dot(ray.direction, ray.direction);
    float k2 = 2 * dot(v, ray.direction);
    float k3 = dot(v, v) - sphere.radius * sphere.radius;

    float d = k2 * k2 - 4 * k1 * k3;

    vec2 temp;

    temp.x = (-k2 + sqrt(d)) / (2 * k1);
    temp.y = (-k2 - sqrt(d)) / (2 * k1);

    return temp;
}

bool closestIntersection(in Ray ray, float zmin, float zmax, out Sphere sphere, inout float t) {
    bool b = false;

    for(int i = 0; i < sphereCount; i++) {
        vec2 tv = intersection(ray, spheres[i]);

        if((tv.x >= zmin && tv.x <= zmax) && tv.x < t) {
            t = tv.x;
            sphere = spheres[i];
            b = true;
        }

        if((tv.y >= zmin && tv.y <= zmax) && tv.y < t) {
            t = tv.y;
            sphere = spheres[i];
            b = true;
        }
    }

    return b;
}

vec3 computeLighting(vec3 P, vec3 N, vec3 V, float specularFactor, vec3 albedo) {
    vec3 light = vec3(0.0);

    for(int i = 0; i < lightCount; i++) {
        vec3 L = normalize(lights[i].position - P);
        vec3 H = normalize(L + V);

        Sphere shadowSphere;
        float shadowT = INFINITY;

        Ray shadowRay;
        shadowRay.position = P;
        shadowRay.direction = lights[i].position - P;

        bool isHit = closestIntersection(shadowRay, 0.001, INFINITY, shadowSphere, shadowT);

        if(isHit) {
            continue;
        }

        float ndotl = dot(N, L);
        float ndoth = dot(N, H);

        vec3 diffuse = albedo * lights[i].color * ndotl;
        vec3 specular = lights[i].color * pow(ndoth, specularFactor * 256.0);

        light += (diffuse + specular) * lights[i].intensity;
    }

    light += vec3(0.1) * albedo;

    return light;
}

vec3 reflect2(vec3 R, vec3 N) {
    return 2.0 * N * dot(N, R) - R;
}

vec3 raytraceRecursion(in Ray ray, float zmin, float zmax, vec3 oldColor, out Ray reflectRay, out float r) {
    Sphere s;
    float t = INFINITY;

    bool isHit = closestIntersection(ray, zmin, zmax, s, t);

    if(!isHit) {
        return oldColor;
    }

    // Lighting
    vec3 P = ray.position + ray.direction * t;
    vec3 N = P - s.position;
    N = normalize(N);
    vec3 color = computeLighting(P, N, -reflectRay.direction, s.specularFactor, s.color);
    
    //float r = s.specularFactor;
    r = s.specularFactor;

    vec3 R = reflect2(-ray.direction, N);

    reflectRay.position = P;
    reflectRay.direction = R;


    return color;
}

vec3 raytraceReflections(in Ray ray, in Sphere sphere, float zmin, float zmax, vec3 P, vec3 N, vec3 color) {
    float r = sphere.specularFactor;

    if(r <= 0) {
        return color;
    }

    vec3 R = reflect2(-ray.direction, N);

    Ray reflectRay;
    reflectRay.position = P;
    reflectRay.direction = R;

    Ray reflectRay2;
    float r2;
    vec3 color2 = raytraceRecursion(reflectRay, zmin, zmax, color, reflectRay2, r2);

    color = color * (1.0 - r) + color2 * r;

    color2 = raytraceRecursion(reflectRay2, zmin, zmax, color, reflectRay, r);

    color = color * (1.0 - r2) + color2 * r2;

    color2 = raytraceRecursion(reflectRay, zmin, zmax, color, reflectRay2, r2);

    color = color * (1.0 - r) + color2 * r;

    return color;
}

vec3 raytrace(in Ray ray, float zmin, float zmax) {

    Sphere sphere;
    float t = INFINITY;

    bool isHit = closestIntersection(ray, zmin, zmax, sphere, t);

    if(!isHit) {
        return vec3(0.0);
    }

    // Lighting
    vec3 P = ray.position + ray.direction * t;
    vec3 N = P - sphere.position;
    N = normalize(N);
    vec3 color = computeLighting(P, N, -ray.direction, sphere.specularFactor, sphere.color);

    // Reflection
    return raytraceReflections(ray, sphere, zmin, zmax, P, N, color);
}

Ray camera_makeRay(vec2 point) {
    vec3 d = camera.forward + point.x * camera.width * camera.right + point.y * camera.height * camera.up;
    Ray ray;
    ray.position = camera.position;
    ray.direction = normalize(d);
    return ray;
}

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 dims = imageSize(out_Color);

    vec2 sc = vec2(
        float(pixelCoords.x * 2) / dims.x - 1.0,
        float(pixelCoords.y * 2) / dims.y - 1.0
    );

    Ray ray = camera_makeRay(sc);


    vec3 color = raytrace(ray, 0.1, 1024.0);

    color = clamp(color, 0.0, 1.0);

    vec4 pixel = vec4(color, 1.0);

    // Do something
    imageStore(out_Color, pixelCoords, pixel);
}