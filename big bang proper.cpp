
#include "stdafx.h"
#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
//Some Windows Headers (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "maths_funcs.h"
//#include "teapot.h" // teapot mesh

// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#define M_PI       3.14159265358979323846   // pi
#define ONE_DEG_IN_RAD (2.0 * M_PI) / 360.0 // 0.017444444
#define PARTICLE_COUNT 1000

using namespace std;
GLuint shaderParticles;

int width = 1200;
int height = 900;


// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS

// Create a NULL-terminated string by reading the provided file
char* readShaderSource(const char* shaderFile) {
	FILE* fp = fopen(shaderFile, "rb"); //!->Why does binary flag "RB" work and not "R"... wierd msvc thing?

	if (fp == NULL) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);
	char* buf = new char[size + 1];
	fread(buf, 1, size, fp);
	buf[size] = '\0';

	fclose(fp);

	return buf;
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
		exit(0);
	}
	const char* pShaderSource = readShaderSource(pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024];
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders(GLuint shaderProgramID, const char *vs, const char *fs)
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0) {
		fprintf(stderr, "Error creating shader program\n");
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, vs, GL_VERTEX_SHADER);
	AddShader(shaderProgramID, fs, GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { 0 };
	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
		exit(1);
	}
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
	glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS

void generateObjectBufferTeapot(GLuint shaderProgramID) {

	// create initial attribute values for particles.
	//-------------------------------------------------
	// generate param for each particles

	float vv[PARTICLE_COUNT * 3]; // start velocities vec3
	float vp[PARTICLE_COUNT * 3]; // start position vec3
	float vt[PARTICLE_COUNT];			// start times
	float t_accum = 0.0f;					// start time
	int j = 0;
	for (int i = 0; i < PARTICLE_COUNT; i++) {

		//https://www.3dgep.com/simulating-particle-effects-using-opengl/
		//sphere emitter
		// + /-
		float sign = (rand() % 2) == 1 ? -1.0f : 1.0f;

		float inclination = sign * (float)(rand() % 180)* ONE_DEG_IN_RAD;
		float azimuth = (float)(rand() % 360) * ONE_DEG_IN_RAD;

		float radius = (float)(rand() % 3);
		float speed = ((float)(rand() % 10)) / 10.0f;

		float sInclination = sinf(inclination);

		float x = sInclination * cosf(azimuth);
		float y = sInclination * sinf(azimuth);
		float z = cosf(azimuth);

		// start times
		vt[i] = t_accum;
		t_accum += 1.0f; // spacing for start times is 0.01 seconds

		vv[j] = x * speed;		 // x
		vv[j + 1] = y * speed;	// y
		vv[j + 2] = z * speed; // z

		vp[j] = x * radius;		 // x
		vp[j + 1] = y * radius;	// y
		vp[j + 2] = z * radius; // z

		j += 3;
	}
	GLuint velocity_vbo = 0;
	glGenBuffers(1, &velocity_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, velocity_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vv), vv, GL_STATIC_DRAW);

	GLuint position_vbo = 0;
	glGenBuffers(1, &position_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vp), vp, GL_STATIC_DRAW);

	GLuint time_vbo = 0;
	glGenBuffers(1, &time_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, time_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vt), vt, GL_STATIC_DRAW);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, velocity_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, time_vbo);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
}


#pragma endregion VBO_FUNCTIONS


void display() {

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//enable particles
	//set up time
	static double previous_seconds = glutGet(GLUT_ELAPSED_TIME);
	double current_seconds = glutGet(GLUT_ELAPSED_TIME);
	double deltaT = (current_seconds - previous_seconds) / 0.01;
	previous_seconds = current_seconds;

	//Render Particles. Enabling point re-sizing in vertex shader
	glEnable(GL_PROGRAM_POINT_SIZE);
	glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);
	//???


	//
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glUseProgram(shaderParticles);


	//Declare your uniform variables that will be used in your shader
	int view_mat_location = glGetUniformLocation(shaderParticles, "V");
	int proj_mat_location = glGetUniformLocation(shaderParticles, "P");
	int emitter_pos_wor_loc = glGetUniformLocation(shaderParticles, "emitter_pos_wor");
	int elapsed_system_time_loc = glGetUniformLocation(shaderParticles, "elapsed_system_time");

	/* update time in shaders */
	glUniform1f(elapsed_system_time_loc, (GLfloat)current_seconds);

	//particles 
	//make up a world position for the emitter
	vec3 emitter_world_pos(0.0f, 0.0f, 4.0f);
	mat4 view = translate(identity_mat4(), vec3(0.0, 0.0, -20.0));
	mat4 persp_proj = perspective(45.0, (float)width / (float)height, 0.1, 100.0);


	// update uniforms & draw
	glViewport(0, 0, width, height);
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
	glUniform3f(emitter_pos_wor_loc, emitter_world_pos.v[0], emitter_world_pos.v[1], emitter_world_pos.v[2]);

	glDrawArrays(GL_POINTS, 0, PARTICLE_COUNT);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	glDisable(GL_PROGRAM_POINT_SIZE);




	//swap buffer for all 
	glutSwapBuffers();
}



void updateScene() {

	// Placeholder code, if you want to work with framerate
	// Wait until at least 16ms passed since start of last frame (Effectively caps framerate at ~60fps)
	static DWORD  last_time = 0;
	DWORD  curr_time = timeGetTime();
	float  delta = (curr_time - last_time) * 0.001f;
	if (delta > 0.03f)
		delta = 0.03f;
	last_time = curr_time;
	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	// Set up the shaders
	shaderParticles = CompileShaders(shaderParticles, "../Shaders/particlesVS.txt", "../Shaders/particlesFS.txt");
	//shaderGreen = CompileShaders(shaderGreen, "../Shaders/PhongBVSGreen.glsl", "../Shaders/PhongBFSGreen.glsl");
	//shaderCookTorrance = CompileShaders(shaderCookTorrance, "../Shaders/CTvs.glsl", "../Shaders/CTfs.glsl");
	// load teapot mesh into a vertex buffer array
	generateObjectBufferTeapot(shaderParticles);



}

int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("Light");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	// glut keyboard callbacks

	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}
	// Set up your objects and shaders
	init();

	// Begin infinite event loop
	glutMainLoop();
	return 0;
}











