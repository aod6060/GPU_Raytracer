#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <GL/glew.h>

int screen_width = 1280;
int screen_height = 720;
bool running = true;

SDL_Window* window = nullptr;
SDL_GLContext context = nullptr;

void app_init();
void app_update(float delta);
void app_render();
void app_release();

int main(int argc, char** argv) {

	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	window = SDL_CreateWindow(
		"GPU Raytracer", 
		SDL_WINDOWPOS_UNDEFINED, 
		SDL_WINDOWPOS_UNDEFINED, 
		screen_width, 
		screen_height, 
		SDL_WINDOW_OPENGL);


	context = SDL_GL_CreateContext(window);

	glewInit();

	SDL_Event e;

	
	app_init();

	uint32_t pre = SDL_GetTicks();
	uint32_t curr = 0;
	float delta = 0.0f;

	// create images
	while (running) {
		curr = SDL_GetTicks();
		delta = (curr - pre) / 1000.0f;
		pre = curr;

		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				running = false;
			}
		}

		app_update(delta);
		app_render();

		SDL_GL_SwapWindow(window);
	}


	app_release();

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}

struct OpenGLCompute;

struct Camera {
	glm::vec3 position;
	glm::vec3 forward;
	glm::vec3 right;
	glm::vec3 up;
	float width;
	float height;
} camera;

struct Compute;

void camera_setup();
void camera_updateCompute(Compute* compute);
void camera_update(float delta);

void sphere_setup(Compute* compute);
void light_setup(Compute* compute);

struct Sphere {
	glm::vec3 position;
	float radius;
	glm::vec3 color;
	// 0.0 = rougher, 1.0 = smoother
	float specularFactor;

	Sphere() {}
	Sphere(glm::vec3 position, float radius, glm::vec3 color, float specularFactor) : position(position), radius(radius), color(color), specularFactor(specularFactor) {}
};

struct Light {
	glm::vec3 position;
	float intensity;
	glm::vec3 color;

	Light() {}
	Light(glm::vec3 position, float intensity, glm::vec3 color) : position(position), intensity(intensity), color(color) {}
};

struct Compute {
	OpenGLCompute* example = nullptr;
	// Output Texture
	uint32_t outputID;
	// Shader
	uint32_t computeShader;
	// Program
	uint32_t program;

	std::map<std::string, uint32_t> uniforms;

	void init(OpenGLCompute* example);
	void compute();
	void release();
};

struct UniformVertex {
	glm::mat4 proj;
	glm::mat4 view;
	glm::mat4 model;
};

struct Scene {
	OpenGLCompute* example = nullptr;
	// Scene Render
	// Shaders
	uint32_t vertexShader;
	uint32_t fragmentShader;
	// Program
	uint32_t program;
	// Uniform Blocks
	UniformVertex uniformVertex;
	uint32_t uniformVertexID;
	uint32_t uniformVertexBlockID;
	// Uniform 
	uint32_t uniformTex;
	// Vertex Array
	uint32_t vertexArray;
	uint32_t vertices = 0;
	uint32_t texCoords = 1;

	// Vertex Buffer
	std::vector<glm::vec3> verticesList;
	uint32_t verticesID;
	std::vector<glm::vec2> texCoordList;
	uint32_t texCoordID;

void init(OpenGLCompute* example);
void render();
void release();
};

struct OpenGLCompute {
	Compute compute;
	// Scene Render
	Scene scene;

	std::string loadFile(std::string path);

	void checkForShaderErrors(uint32_t id);
	void checkForProgramErrors(uint32_t id);

	void init();

	void render();

	void release();

} glcompute;

void app_init() {
	camera_setup();
	glcompute.init();

}

void app_update(float delta) {
	camera_update(delta);
}

void app_render() {
	glClear(GL_COLOR_BUFFER_BIT);
	glcompute.render();
}

void app_release() {
	glcompute.release();
}

float yaw = 0.0f;
float pitch = 0.0f;

void camera_setup() {
	float fov = 60.0f;
	float aspect = (float)screen_width / (float)screen_height;

	camera.position = glm::vec3(0.0f, 0.0f, 0.0f);


	camera.forward = glm::normalize(glm::vec3(0, 0, -1));
	camera.right = glm::normalize(glm::cross(camera.forward, glm::vec3(0.0f, 1.0f, 0.0f)));
	camera.up = glm::cross(camera.forward, camera.right);

	std::cout << camera.up.x << ", " << camera.up.y << ", " << camera.up.z << std::endl;

	camera.height = tan(fov);
	camera.width = camera.height * aspect;
}

void camera_updateCompute(Compute* compute) {
	glUniform3f(compute->uniforms["camera.position"], camera.position.x, camera.position.y, camera.position.z);
	glUniform3f(compute->uniforms["camera.forward"], camera.forward.x, camera.forward.y, camera.forward.z);
	glUniform3f(compute->uniforms["camera.right"], camera.right.x, camera.right.y, camera.right.z);
	glUniform3f(compute->uniforms["camera.up"], camera.up.x, camera.up.y, camera.up.z);
	glUniform1f(compute->uniforms["camera.width"], camera.width);
	glUniform1f(compute->uniforms["camera.height"], camera.height);
}

#define MAX_SPHERE_SIZE 20

void sphere_setup(Compute* compute) {
	std::vector<Sphere> spheres = {
		Sphere(glm::vec3(-8, 0, 0), 1, glm::vec3(0.5f, 0.5f, 0.5f), 0.5),
		Sphere(glm::vec3(0, 0, -8), 1, glm::vec3(0.0f, 0.5f, 0.0f), 0.5),
		Sphere(glm::vec3(8, 0, 0), 1, glm::vec3(0.0f, 0.0f, 0.5f), 0.5),
		Sphere(glm::vec3(0, 0, 8), 1, glm::vec3(0.5f, 0.0f, 0.0f), 0.5),
		Sphere(glm::vec3(0, -5001, 0), 5000, glm::vec3(0.5f, 0.5f, 0.0f), 0.5)
	};

	for (int i = 0; i < spheres.size(); i++) {
		std::stringstream ss;
		ss << "spheres[" << i << "].position";
		//compute->uniforms[ss.str()] = glGetUniformLocation(compute->program, ss.str().c_str());
		glUniform3f(compute->uniforms[ss.str()], spheres[i].position.x, spheres[i].position.y, spheres[i].position.z);
		ss.str("");

		ss << "spheres[" << i << "].radius";
		//compute->uniforms[ss.str()] = glGetUniformLocation(compute->program, ss.str().c_str());
		glUniform1f(compute->uniforms[ss.str()], spheres[i].radius);
		ss.str("");

		ss << "spheres[" << i << "].color";
		//compute->uniforms[ss.str()] = glGetUniformLocation(compute->program, ss.str().c_str());
		glUniform3f(compute->uniforms[ss.str()], spheres[i].color.x, spheres[i].color.y, spheres[i].color.z);
		ss.str("");

		ss << "spheres[" << i << "].specularFactor";
		//compute->uniforms[ss.str()] = glGetUniformLocation(compute->program, ss.str().c_str());
		glUniform1f(compute->uniforms[ss.str()], spheres[i].specularFactor);
		ss.str("");
	}

	if (spheres.size() < MAX_SPHERE_SIZE) {
		glUniform1i(compute->uniforms["sphereCount"], spheres.size());
	}
	else {
		glUniform1i(compute->uniforms["sphereCount"], MAX_SPHERE_SIZE);
	}
}

#define MAX_LIGHT_SIZE 8

void light_setup(Compute* compute) {
	std::vector<Light> lights = {
		Light(glm::vec3(0, 1, 0), 0.6, glm::vec3(1.0f))
	};

	for (int i = 0; i < lights.size(); i++) {
		std::stringstream ss;
		ss << "lights[" << i << "].position";
		glUniform3f(compute->uniforms[ss.str()], lights[i].position.x, lights[i].position.y, lights[i].position.z);
		ss.str("");
		ss << "lights[" << i << "].intensity";
		glUniform1f(compute->uniforms[ss.str()], lights[i].intensity);
		ss.str("");
		ss << "lights[" << i << "].color";
		glUniform3f(compute->uniforms[ss.str()], lights[i].color.x, lights[i].color.y, lights[i].color.z);
		ss.str("");
	}

	if (lights.size() < MAX_LIGHT_SIZE) {
		glUniform1i(compute->uniforms["lightCount"], lights.size());
	}
	else {
		glUniform1i(compute->uniforms["lightCount"], MAX_LIGHT_SIZE);
	}
}

void camera_update(float delta) {
	const uint8_t* keys = SDL_GetKeyboardState(nullptr);

	float rotSpeed = 64.0f;

	if (keys[SDL_SCANCODE_LEFT]) {
		yaw -= rotSpeed * delta;
	}

	if (keys[SDL_SCANCODE_RIGHT]) {
		yaw += rotSpeed * delta;
	}

	if (yaw < -360.0f) {
		yaw += 360.0f;
	}

	if (yaw > 360.0f) {
		yaw -= 360.0f;
	}

	if (keys[SDL_SCANCODE_UP]) {
		pitch += rotSpeed * delta;
	}

	if (keys[SDL_SCANCODE_DOWN]) {
		pitch -= rotSpeed * delta;
	}


	if (pitch < -90.0f) {
		pitch = -90.0f;
	}

	if (pitch > 90.0f) {
		pitch = 90.0f;
	}

	glm::vec3 direction = glm::vec3(
		glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch)),
		glm::sin(glm::radians(pitch)),
		glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch))
	);

	camera.forward = glm::normalize(direction);
	camera.right = glm::normalize(glm::cross(camera.forward, glm::vec3(0.0f, 1.0f, 0.0f)));
	camera.up = glm::cross(camera.forward, camera.right);


	glm::vec3 forward = glm::vec3(
		camera.forward.x,
		0.0f,
		camera.forward.z);

	float speed = 4.0f;

	if (keys[SDL_SCANCODE_W]) {
		camera.position += speed * delta * forward;
	}

	if (keys[SDL_SCANCODE_S]) {
		camera.position -= speed * delta * forward;
	}

	if (keys[SDL_SCANCODE_A]) {
		camera.position -= speed * delta * camera.right;
	}

	if (keys[SDL_SCANCODE_D]) {
		camera.position += speed * delta * camera.right;
	}

	if (keys[SDL_SCANCODE_SPACE]) {
		camera.position.y += speed * delta;
	}

	if (keys[SDL_SCANCODE_LSHIFT]) {
		camera.position.y -= speed * delta;
	}
}

void OpenGLCompute::init() {
	this->compute.init(this);
	this->scene.init(this);
}

void OpenGLCompute::render() {
	// Compute Raytracer...
	this->compute.compute();
	// Render Scene...
	this->scene.render();
}

void OpenGLCompute::release() {
	this->scene.release();
	this->compute.release();
}

// Compute
void Compute::init(OpenGLCompute* example) {
	this->example = example;

	// Create Image
	glGenTextures(1, &this->outputID);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, this->outputID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, screen_width, screen_height, 0, GL_RGBA, GL_FLOAT, 0);

	glBindImageTexture(0, this->outputID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	int work_grp_cnt[3];

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);

	std::cout << "max global (total) work group counts x: "<< work_grp_cnt[0]<<" y: " << work_grp_cnt[1] << " z: " << work_grp_cnt[2] << " " << std::endl;

	int work_grp_size[3];

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);

	std::cout << "max global (total) work group counts x: " << work_grp_size[0] << " y: " << work_grp_size[1] << " z: " << work_grp_size[2] << " " << std::endl;

	int work_grp_inv;
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
	std::cout << "max local work group invocations " << work_grp_inv << std::endl;

	// Shader
	this->computeShader = glCreateShader(GL_COMPUTE_SHADER);
	std::string src = this->example->loadFile("raytracer.glsl");
	const char* c_src = src.c_str();
	glShaderSource(this->computeShader, 1, &c_src, nullptr);
	glCompileShader(this->computeShader);
	this->example->checkForShaderErrors(this->computeShader);

	// Program
	this->program = glCreateProgram();
	glAttachShader(this->program, this->computeShader);
	glLinkProgram(this->program);
	this->example->checkForProgramErrors(this->program);

	glUseProgram(this->program);
	// Setup Uniforms
	this->uniforms["camera.position"]		= glGetUniformLocation(this->program, "camera.position");
	std::cout << this->uniforms["camera.position"] << std::endl;
	this->uniforms["camera.forward"]		= glGetUniformLocation(this->program, "camera.forward");
	std::cout << this->uniforms["camera.forward"] << std::endl;
	this->uniforms["camera.right"]			= glGetUniformLocation(this->program, "camera.right");
	std::cout << this->uniforms["camera.right"] << std::endl;
	this->uniforms["camera.up"]				= glGetUniformLocation(this->program, "camera.up");
	std::cout << this->uniforms["camera.up"] << std::endl;
	this->uniforms["camera.width"]			= glGetUniformLocation(this->program, "camera.width");
	std::cout << this->uniforms["camera.width"] << std::endl;
	this->uniforms["camera.height"]			= glGetUniformLocation(this->program, "camera.height");
	std::cout << this->uniforms["camera.height"] << std::endl;

	// Spheres
	for (int i = 0; i < MAX_SPHERE_SIZE; i++) {
		std::stringstream ss;
		ss << "spheres["<<i<<"].position";
		this->uniforms[ss.str()] = glGetUniformLocation(this->program, ss.str().c_str());
		ss.str("");

		ss << "spheres[" << i << "].radius";
		this->uniforms[ss.str()] = glGetUniformLocation(this->program, ss.str().c_str());
		ss.str("");

		ss << "spheres[" << i << "].color";
		this->uniforms[ss.str()] = glGetUniformLocation(this->program, ss.str().c_str());
		ss.str("");

		ss << "spheres[" << i << "].specularFactor";
		this->uniforms[ss.str()] = glGetUniformLocation(this->program, ss.str().c_str());
		ss.str("");
	}

	this->uniforms["sphereCount"] = glGetUniformLocation(this->program, "sphereCount");

	// Light
	for (int i = 0; i < MAX_LIGHT_SIZE; i++) {
		std::stringstream ss;
		ss << "lights["<<i<<"].position";
		this->uniforms[ss.str()] = glGetUniformLocation(this->program, ss.str().c_str());
		ss.str("");

		ss << "lights[" << i << "].intensity";
		this->uniforms[ss.str()] = glGetUniformLocation(this->program, ss.str().c_str());
		ss.str("");

		ss << "lights[" << i << "].color";
		this->uniforms[ss.str()] = glGetUniformLocation(this->program, ss.str().c_str());
		ss.str("");
	}

	this->uniforms["lightCount"] = glGetUniformLocation(this->program, "lightCount");

	sphere_setup(this);
	light_setup(this);

	glUseProgram(0);

	int size;
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &size);


	std::cout << "Max Uniform, My Structure Size" << std::endl;
	std::cout << size << ", " << sizeof(Camera) << std::endl;
}

void Compute::compute() {
	
	glUseProgram(this->program);

	camera_updateCompute(this);

	glDispatchCompute(screen_width, screen_height, 1);
	glUseProgram(0);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void Compute::release() {
	glDetachShader(this->program, this->computeShader);
	glDeleteProgram(this->program);
	glDeleteShader(this->computeShader);
	glDeleteTextures(1, &this->outputID);
	this->example = nullptr;
}

// Scene
void Scene::init(OpenGLCompute* example) {
	this->example = example;

	// Shaders
	// Vertex
	this->vertexShader = glCreateShader(GL_VERTEX_SHADER);
	std::string src = this->example->loadFile("main.vert.glsl");
	const char* c_src = src.c_str();
	std::cout << c_src << std::endl;
	glShaderSource(this->vertexShader, 1, &c_src, nullptr);
	glCompileShader(this->vertexShader);
	this->example->checkForShaderErrors(this->vertexShader);

	// Fragment
	this->fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	src = this->example->loadFile("main.frag.glsl");
	c_src = src.c_str();
	std::cout << c_src << std::endl;
	glShaderSource(this->fragmentShader, 1, &c_src, nullptr);
	glCompileShader(this->fragmentShader);
	this->example->checkForShaderErrors(this->fragmentShader);
	
	// Program
	this->program = glCreateProgram();
	glAttachShader(this->program, this->vertexShader);
	glAttachShader(this->program, this->fragmentShader);
	glLinkProgram(this->program);
	this->example->checkForProgramErrors(this->program);

	glGenBuffers(1, &this->uniformVertexID);
	glBindBuffer(GL_UNIFORM_BUFFER, this->uniformVertexID);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformVertex), nullptr, GL_DYNAMIC_DRAW);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, this->uniformVertexID, 0, sizeof(UniformVertex));
	//glBindBufferRange(GL_UNIFORM_BUFFER, 0, this->uniformVertexID, 0, sizeof(UniformVertex));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glGenVertexArrays(1, &this->vertexArray);

	glUseProgram(this->program);

	glBindVertexArray(this->vertexArray);
	glEnableVertexAttribArray(this->vertices);
	glEnableVertexAttribArray(this->texCoords);
	glBindVertexArray(0);
	glDisableVertexAttribArray(this->vertices);
	glDisableVertexAttribArray(this->texCoords);

	glUseProgram(0);


	// Vertex Buffers
	this->verticesList.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
	this->verticesList.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
	this->verticesList.push_back(glm::vec3(0.0f, 1.0f, 0.0f));

	this->verticesList.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
	this->verticesList.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
	this->verticesList.push_back(glm::vec3(1.0f, 1.0f, 0.0f));

	glGenBuffers(1, &this->verticesID);
	glBindBuffer(GL_ARRAY_BUFFER, this->verticesID);
	glBufferData(GL_ARRAY_BUFFER, this->verticesList.size() * sizeof(glm::vec3), this->verticesList.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// TexCoords
	this->texCoordList.push_back(glm::vec2(0.0f, 0.0f));
	this->texCoordList.push_back(glm::vec2(1.0f, 0.0f));
	this->texCoordList.push_back(glm::vec2(0.0f, 1.0f));

	this->texCoordList.push_back(glm::vec2(0.0f, 1.0f));
	this->texCoordList.push_back(glm::vec2(1.0f, 0.0f));
	this->texCoordList.push_back(glm::vec2(1.0f, 1.0f));

	glGenBuffers(1, &this->texCoordID);
	glBindBuffer(GL_ARRAY_BUFFER, this->texCoordID);
	glBufferData(GL_ARRAY_BUFFER, this->texCoordList.size() * sizeof(glm::vec2), this->texCoordList.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

}

void Scene::render() {
	this->uniformVertex.proj = glm::ortho(0.0f, (float)screen_width, (float)screen_height, 0.0f);
	this->uniformVertex.view = glm::mat4(1.0f);
	this->uniformVertex.model =
		glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)) *
		glm::scale(glm::mat4(1.0f), glm::vec3((float)screen_width, (float)screen_height, 0.0f));

	glUseProgram(this->program);

	glBindTexture(GL_TEXTURE_2D, this->example->compute.outputID);

	glBindBuffer(GL_UNIFORM_BUFFER, this->uniformVertexID);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(UniformVertex), &this->uniformVertex);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);


	glBindVertexArray(this->vertexArray);

	glBindBuffer(GL_ARRAY_BUFFER, this->verticesID);
	glVertexAttribPointer(this->vertices, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, this->texCoordID);
	glVertexAttribPointer(this->texCoords, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDrawArrays(GL_TRIANGLES, 0, this->verticesList.size());
	glBindVertexArray(0);

	glBindTexture(GL_TEXTURE_2D, 0);

	glUseProgram(0);
}

void Scene::release() {
	glDeleteBuffers(1, &this->texCoordID);
	glDeleteBuffers(1, &this->verticesID);
	glDeleteVertexArrays(1, &this->vertexArray);
	glDeleteBuffers(1, &this->uniformVertexID);
	glDetachShader(this->program, this->vertexShader);
	glDetachShader(this->program, this->fragmentShader);
	glDeleteProgram(this->program);
	glDeleteShader(this->fragmentShader);
	glDeleteShader(this->vertexShader);
	this->example = nullptr;
}

std::string OpenGLCompute::loadFile(std::string path) {
	std::ifstream in(path);
	std::stringstream ss;
	std::string temp;
	while (std::getline(in, temp)) {
		ss << temp << std::endl;
	}
	in.close();
	return ss.str();
}

void OpenGLCompute::checkForShaderErrors(uint32_t id) {
	char log[1024];
	int len;

	glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);

	if (len > 0) {
		glGetShaderInfoLog(id, len, nullptr, log);
		std::cout << log << std::endl;
	}
}

void OpenGLCompute::checkForProgramErrors(uint32_t id) {
	char log[1024];
	int len;

	glGetProgramiv(id, GL_INFO_LOG_LENGTH, &len);

	if (len > 0) {
		glGetProgramInfoLog(id, len, nullptr, log);
		std::cout << log << std::endl;
	}
}
