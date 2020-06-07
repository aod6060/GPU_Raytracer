#version 430

layout(location = 0) in vec3 vertices;
layout(location = 1) in vec2 texCoords;

layout(std140, binding=0) uniform UniformVertex {
    mat4 proj;
    mat4 view;
    mat4 model;
};

layout(location = 2) out vec2 v_TexCoords;

void main() {
    gl_Position = proj * view * model * vec4(vertices, 1.0);
    v_TexCoords = texCoords;
}