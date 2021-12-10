﻿
/** @file Week14-1-BlendingDemo.cpp
 *  @brief Blending Demo and Alpha Compositing
 *  In computer graphics, alpha compositing is the process of combining an image with a background to create the appearance of partial or full transparency.
 *  When alpha blending is enabled in the OpenGL pipeline, we use this form for their blending:
 *	DestinationColor.rgb = (SourceColor.rgb * SourceColor.a) + (DestinationColor.rgb * (1 - SourceColor.a));
 *  SourceColor: the source color vector. This is the color output of the fragment shader.
 *  DestinationColor: the destination color vector. This is the color vector that is currently stored in the color buffer
 *  Now that we have enabled blending we need to tell OpenGL how it should actually blend.
 *  Imagine we have two triangles where we want to draw the semi-transparent blue triangle on top of the yellow triangle.
 *  The yellow triangle will be the destination color (and therefore should be first in the color buffer)
 *  and we are now going to draw the blue triangle over the yellow triangle.
 *  If the blue triangle contributes 60% to the final color we want the yellow triangle to contribute 40% of the final color (e.g. 1.0 - 0.6).
 *  color(RGBA) =(1.0,1.0,0.0,0.6)∗0.6+(0.0,0.0,1.0,1.0)∗(1−0.6)
 *  But how do we actually tell OpenGL to use factors? There is a function for this called glBlendFunc.
 *  Syntax: void glBlendFunc(sfactor, dfactor);
 *  Parameters:
 *  sfactor:A GLenum specifying a multiplier for the source blending factors. The default value is GL_ONE.
 *  dfactor: A GLenum specifying a multiplier for the destination blending factors. The default value is GL_ZERO.
 *  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

 *  @note press WASD for tracking the camera or zooming in and out
 *  @note press arrow keys and page up and page down to move the light
 *  @note move mouse to yaw and pitch
 *  @attention we are using directional vertex and fragment shaders!
 *  @author Hooman Salamat
 *  @bug No known bugs.
 */
using namespace std;

#include "stdlib.h"
#include "time.h"
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "prepShader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <iostream>
#include "Shape.h"
#include "Light.h"
#include "Texture.h"
#include "MazeShape.h"

#define BUFFER_OFFSET(x)  ((const void*) (x))
#define FPS 60
#define MOVESPEED 0.1f
#define TURNSPEED 0.05f
#define X_AXIS glm::vec3(1,0,0)
#define Y_AXIS glm::vec3(0,1,0)
#define Z_AXIS glm::vec3(0,0,1)
#define XY_AXIS glm::vec3(1,0.9,0)
#define YZ_AXIS glm::vec3(0,1,1)
#define XZ_AXIS glm::vec3(1,0,1)
#define SPEED 0.25f

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

enum keyMasks {
	KEY_FORWARD = 0b00000001,		// 0x01 or   1	or   01
	KEY_BACKWARD = 0b00000010,		// 0x02 or   2	or   02
	KEY_LEFT = 0b00000100,
	KEY_RIGHT = 0b00001000,
	KEY_UP = 0b00010000,
	KEY_DOWN = 0b00100000,
	KEY_MOUSECLICKED = 0b01000000

	// Any other keys you want to add.
};

static unsigned int
program,
vertexShaderId,
fragmentShaderId;

GLuint modelID, viewID, projID;
glm::mat4 View, Projection;

// Our bitflag variable. 1 byte for up to 8 key states.
unsigned char keys = 0; // Initialized to 0 or 0b00000000.

// Texture variables.
GLuint blankID;
GLint width, height, bitDepth;


//lightposition (this is the sphere's position!)
glm::vec3 directionalLightPosition = glm::vec3(8.0f, 10.0f, 0.0f);

// Light objects. Now OOP.
AmbientLight aLight(
	glm::vec3(1.0f, 1.0f, 1.0f),	// Diffuse color.
	0.5f);							// Diffuse strength.

DirectionalLight dLight(
	glm::vec3(0.0f, 0.0f, 1.0f),	// direction using the origin
  //directionalLightPosition,
	glm::vec3(1.0f, 1.0f, 1.0f),	// Diffuse color.
	1.0f);							// Diffuse strength.

Material mat = { 0.5f, 8 }; // Alternate way to construct an object.

// Camera and transform variables.
float scale = 1.0f, angle = 0.0f;
glm::vec3 position, frontVec, worldUp, upVec, rightVec; // Set by function
GLfloat pitch, yaw;
int lastX, lastY;

// Geometry data.
Grid g_grid(16,3);
Cube g_cube;
Prism g_prism(24);
Sphere g_sphere(5);
Cone g_cone(100);
MazeShape rectangle;

void timer(int); // Prototype.

Texture* pTexture = NULL;
Texture* blankTexture = NULL;
Texture* waterTexture = NULL;
GLuint textureID;

void resetView()
{
	position = glm::vec3(8.0f, 5.0f, 20.0f);
	frontVec = glm::vec3(0.0f, 0.0f, -1.0f);
	worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	pitch = 0.0f;
	yaw = -90.0f;
}


void loadTextures()
{
	glUniform1i(glGetUniformLocation(program, "texture0"), 0);
	pTexture = new Texture(GL_TEXTURE_2D, "Media/bodyMetal.bmp", GL_RGB);
	pTexture->Bind(GL_TEXTURE0);
	pTexture->Load();

	blankTexture = new Texture(GL_TEXTURE_2D, "Media/blank.jpg", GL_RGB);
	blankTexture->Bind(GL_TEXTURE0);
	blankTexture->Load();

	//! attention: water picture has alpha channel!
	waterTexture = new Texture(GL_TEXTURE_2D, "Media/Water03.png", GL_RGBA);
	waterTexture->Bind(GL_TEXTURE0);
	waterTexture->Load();

}

void setupLights()
{
	// Setting material values.
	glUniform1f(glGetUniformLocation(program, "mat.specularStrength"), mat.specularStrength);
	glUniform1f(glGetUniformLocation(program, "mat.shininess"), mat.shininess);

	// Setting ambient light.
	glUniform3f(glGetUniformLocation(program, "aLight.base.diffuseColor"), aLight.diffuseColor.x, aLight.diffuseColor.y, aLight.diffuseColor.z);
	glUniform1f(glGetUniformLocation(program, "aLight.base.diffuseStrength"), aLight.diffuseStrength);

	// Setting directional light.
	glUniform3f(glGetUniformLocation(program, "dLight.base.diffuseColor"), dLight.diffuseColor.x, dLight.diffuseColor.y, dLight.diffuseColor.z);
	glUniform1f(glGetUniformLocation(program, "dLight.base.diffuseStrength"), dLight.diffuseStrength);
	glUniform3f(glGetUniformLocation(program, "dLight.direction"), dLight.direction.x, dLight.direction.y, dLight.direction.z);

}

void setupVAOs()
{
	// All VAO/VBO data now in Shape.h! But we still need to do this AFTER OpenGL is initialized.
	g_grid.BufferShape();
	g_cube.BufferShape();
	g_prism.BufferShape();
	g_sphere.BufferShape();
	g_cone.BufferShape();

	rectangle.setModelID(&modelID);
	rectangle.addShape(Cube(5,2,5), { glm::vec3(0,0,0) ,glm::vec3(5,2,5),glm::vec3(1,0,0),0 });
	//rectangle.addShape(Cube(), { glm::vec3(0,0,-1) ,glm::vec3(1,1,1),glm::vec3(1,0,0),0 });
	//rectangle.addShape(Cube(), { glm::vec3(0,1,0) ,glm::vec3(1,1,1),glm::vec3(1,0,0),0 });
}

void setupShaders()
{
	// Create shader program executable.
	vertexShaderId = setShader((char*)"vertex", (char*)"directional.vert");
	fragmentShaderId = setShader((char*)"fragment", (char*)"directional.frag");
	program = glCreateProgram();
	glAttachShader(program, vertexShaderId);
	glAttachShader(program, fragmentShaderId);
	glLinkProgram(program);

	GLint Success;
	glGetProgramiv(program, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		char temp[1024];
		glGetProgramInfoLog(program, 1024, 0, temp);
		fprintf(stderr, "Failed to link program:\n%s\n", temp);
		glDeleteProgram(program);
		program = 0;
		exit(EXIT_FAILURE);
	}

	glValidateProgram(program);
	glGetProgramiv(program, GL_VALIDATE_STATUS, &Success);
	if (Success == 0) {
		char temp[1024];
		glGetProgramInfoLog(program, 1024, 0, temp);
		fprintf(stderr, "Invalid Shader program:\n%s\n", temp);
		glDeleteProgram(program);
		program = 0;
		exit(EXIT_FAILURE);
	}
	glUseProgram(program);

	modelID = glGetUniformLocation(program, "model");
	viewID = glGetUniformLocation(program, "view");
	projID = glGetUniformLocation(program, "projection");
}

void init(void)
{
	srand((unsigned)time(NULL));

	// Projection matrix : 45∞ Field of View, 1:1 ratio, display range : 0.1 unit <-> 100 units
	Projection = glm::perspective(glm::radians(45.0f), 1.0f / 1.0f, 0.1f, 100.0f);
	// Or, for an ortho camera :
	// Projection = glm::ortho(-3.0f, 3.0f, -3.0f, 3.0f, 0.0f, 100.0f); // In world coordinates

	setupShaders();

	// Camera matrix
	resetView();

	loadTextures();

	setupLights();

	setupVAOs();

	glUniformMatrix4fv(projID, 1, GL_FALSE, &Projection[0][0]);

	// Enable depth testing and face culling.
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	//glBlendFunc(GL_ONE, GL_ZERO);

	//we want to take the alpha of the source color vector for the source factor and 1−alpha of the same color vector for the destination factor.
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//It is also possible to set different options for the RGB and alpha channel individually
	//glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,   GL_ZERO, GL_ZERO );

	//glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);

	//glBlendFunc(GL_SRC_COLOR, GL_DST_COLOR);

	//pick one of the blend function
	//glBlendColor(1.0, 0.0, 0.0, 1.0);
	//glBlendFunc(GL_CONSTANT_COLOR, GL_DST_COLOR);
	//glBlendFunc( GL_SRC_COLOR, GL_CONSTANT_COLOR);


	//glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_SRC_ALPHA_SATURATE);


	//Src+Dst
	//glBlendEquation(GL_FUNC_ADD);

	//Src−Dst
	//glBlendEquation(GL_FUNC_SUBTRACT);

	//Dst−Src
	//glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);

	//min(Dst, Src).
	//glBlendEquation(GL_MIN);

	 //max(Dst,Src).
	//glBlendEquation(GL_MAX);


	// Enable smoothing.
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);


	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	timer(0); // Setup my recursive 'fixed' timestep/framerate.
}

//---------------------------------------------------------------------
//
// calculateView
//
void calculateView()
{
	frontVec.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	frontVec.y = sin(glm::radians(pitch));
	frontVec.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	frontVec = glm::normalize(frontVec);
	rightVec = glm::normalize(glm::cross(frontVec, worldUp));
	upVec = glm::normalize(glm::cross(rightVec, frontVec));

	View = glm::lookAt(
		position, // Camera position
		position + frontVec, // Look target
		upVec); // Up vector
}

//---------------------------------------------------------------------
//
// transformModel
//
void transformObject(glm::vec3 scale, glm::vec3 rotationAxis, float rotationAngle, glm::vec3 translation) {
	glm::mat4 Model;
	Model = glm::mat4(1.0f);
	Model = glm::translate(Model, translation);
	Model = glm::rotate(Model, glm::radians(rotationAngle), rotationAxis);
	Model = glm::scale(Model, scale);

	// We must now update the View.
	//calculateView();

	glUniformMatrix4fv(modelID, 1, GL_FALSE, &Model[0][0]);
	//glUniformMatrix4fv(viewID, 1, GL_FALSE, &View[0][0]);
	//glUniformMatrix4fv(projID, 1, GL_FALSE, &Projection[0][0]);
}

//---------------------------------------------------------------------
//
// display
//
void display(void)
{
	calculateView();
	glUniformMatrix4fv(viewID, 1, GL_FALSE, &View[0][0]);
	//you need this function here as light values might change
	setupLights();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glBindTexture(GL_TEXTURE_2D, blankID); // Use this texture for all shapes.


	// Grid.
	blankTexture->Bind(GL_TEXTURE0);
	g_grid.RecolorShape(1.0, 0.0, 1.0);
	transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	g_grid.DrawShape(GL_LINE_LOOP);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Cube.
	waterTexture->Bind(GL_TEXTURE0);
	g_cube.RecolorShape(0.0, 1.0, 1.0);
	transformObject(glm::vec3(1, 1, 1), X_AXIS, 0.0f, glm::vec3(0, 0, 0));
	//transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(8.0f, 2.0f, -1.0f));
	g_cube.DrawShape(GL_TRIANGLES);
	glBindTexture(GL_TEXTURE_2D, 0);

	angle += 2.0f;

	// Sphere.
	pTexture->Bind(GL_TEXTURE0);
	g_prism.RecolorShape(1.0, 1.0, 0.0);
	transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, angle, directionalLightPosition);
	g_sphere.DrawShape(GL_TRIANGLES);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Prism.
	blankTexture->Bind(GL_TEXTURE0);
	g_prism.RecolorShape(0.0, 1.0, 0.0);
	transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(10.0f, 0.0f, -2.0f));
	glUniform1f(glGetUniformLocation(program, "mat.specularStrength"), 1.0f);
	glUniform1f(glGetUniformLocation(program, "mat.shininess"), 128);
	g_prism.DrawShape(GL_TRIANGLES);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Prism.
	blankTexture->Bind(GL_TEXTURE0);
	//g_prism.RecolorShape(0.0, 1.0, 0.0);
	transformObject(glm::vec3(1.0f, 1.0f, 1.0f), Y_AXIS, 45.0f, glm::vec3(10.0f, 0.5f, -6.0f));
	g_cone.DrawShape(GL_TRIANGLES);
	glBindTexture(GL_TEXTURE_2D, 0);

	//waterTexture->Bind(GL_TEXTURE0);
	rectangle.draw({ 3, 0, -5 }, waterTexture);
	//glBindTexture(GL_TEXTURE_2D, 0);

	glutSwapBuffers(); // Now for a potentially smoother render.
}

void idle() // Not even called.
{
	glutPostRedisplay();
}

void parseKeys()
{
	if (keys & KEY_FORWARD)
		position += frontVec * MOVESPEED;
	if (keys & KEY_BACKWARD)
		position -= frontVec * MOVESPEED;
	if (keys & KEY_LEFT)
		position -= rightVec * MOVESPEED;
	if (keys & KEY_RIGHT)
		position += rightVec * MOVESPEED;
	if (keys & KEY_UP)
		position += upVec * MOVESPEED;
	if (keys & KEY_DOWN)
		position -= upVec * MOVESPEED;
}

void timer(int) { // Tick of the frame.
	// Get first timestamp
	int start = glutGet(GLUT_ELAPSED_TIME);
	// Update call
	parseKeys();
	// Display call
	glutPostRedisplay();
	// Calling next tick
	int end = glutGet(GLUT_ELAPSED_TIME);
	glutTimerFunc((1000 / FPS) - (end - start), timer, 0);
}

// Keyboard input processing routine.
void keyDown(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27:
		exit(0);
		break;
	case 'w':
		if (!(keys & KEY_FORWARD))
			keys |= KEY_FORWARD; // keys = keys | KEY_FORWARD
		break;
	case 's':
		if (!(keys & KEY_BACKWARD))
			keys |= KEY_BACKWARD;
		break;
	case 'a':
		if (!(keys & KEY_LEFT))
			keys |= KEY_LEFT;
		break;
	case 'd':
		if (!(keys & KEY_RIGHT))
			keys |= KEY_RIGHT;
		break;
	case 'r':
		if (!(keys & KEY_UP))
			keys |= KEY_UP;
		break;
	case 'f':
		if (!(keys & KEY_DOWN))
			keys |= KEY_DOWN;
		break;
	default:
		break;
	}
}

void keyDownSpec(int key, int x, int y) // x and y is mouse location upon key press.
{
	switch (key)
	{
	case GLUT_KEY_UP: // Up arrow.
		directionalLightPosition.y += 1 * MOVESPEED;
		//dLight.direction = directionalLightPosition;
		break;
	case GLUT_KEY_DOWN: // Down arrow.
		directionalLightPosition.y -= 1 * MOVESPEED;
		//dLight.direction = directionalLightPosition;
		break;
	case GLUT_KEY_LEFT: // Left arrow.
		directionalLightPosition.x -= 1 * MOVESPEED;
		//dLight.direction = directionalLightPosition;
		break;
	case GLUT_KEY_RIGHT: // DoRightwn arrow.
		directionalLightPosition.x += 1 * MOVESPEED;
		//dLight.direction = directionalLightPosition;
		break;
	case GLUT_KEY_PAGE_UP: // PAGE UP.
		directionalLightPosition.z -= 1 * MOVESPEED;
		//dLight.direction = directionalLightPosition;
		break;
	case GLUT_KEY_PAGE_DOWN: // PAGE DOWN.
		directionalLightPosition.z += 1 * MOVESPEED;
		//dLight.direction = directionalLightPosition;
		break;
	default:
		break;
	}
}

void keyUp(unsigned char key, int x, int y) // x and y is mouse location upon key press.
{
	switch (key)
	{
	case 'w':
		keys &= ~KEY_FORWARD; // keys = keys & ~KEY_FORWARD. ~ is bitwise NOT.
		break;
	case 's':
		keys &= ~KEY_BACKWARD;
		break;
	case 'a':
		keys &= ~KEY_LEFT;
		break;
	case 'd':
		keys &= ~KEY_RIGHT;
		break;
	case 'r':
		keys &= ~KEY_UP;
		break;
	case 'f':
		keys &= ~KEY_DOWN;
		break;
	case ' ':
		resetView();
		break;
	default:
		break;
	}
}

void keyUpSpec(int key, int x, int y) // x and y is mouse location upon key press.
{
	switch (key)
	{
	case GLUT_KEY_UP:

		break;
	case GLUT_KEY_DOWN:

		break;
	default:
		break;
	}
}

void mouseMove(int x, int y)
{
	if (keys & KEY_MOUSECLICKED)
	{
		pitch -= (GLfloat)((y - lastY) * TURNSPEED);
		yaw += (GLfloat)((x - lastX) * TURNSPEED);
		lastY = y;
		lastX = x;
	}
}

void mouseClick(int btn, int state, int x, int y)
{
	if (state == 0)
	{
		lastX = x;
		lastY = y;
		keys |= KEY_MOUSECLICKED; // Flip flag to true
		glutSetCursor(GLUT_CURSOR_NONE);
		//cout << "Mouse clicked." << endl;
	}
	else
	{
		keys &= ~KEY_MOUSECLICKED; // Reset flag to false
		glutSetCursor(GLUT_CURSOR_INHERIT);
		//cout << "Mouse released." << endl;
	}
}

//---------------------------------------------------------------------
//
// clean
//
void clean()
{
	cout << "Cleaning up!" << endl;
	glDeleteTextures(1, &blankID);
}

//---------------------------------------------------------------------
//
// main
//
int main(int argc, char** argv)
{
	//Before we can open a window, theremust be interaction between the windowing systemand OpenGL.In GLUT, this interaction is initiated by the following function call :
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
	glutSetOption(GLUT_MULTISAMPLE, 8);

	//if you comment out this line, a window is created with a default size
	glutInitWindowSize(1024, 1024);

	//the top-left corner of the display
	glutInitWindowPosition(0, 0);

	glutCreateWindow("Blending Demo");

	glewInit();	//Initializes the glew and prepares the drawing pipeline.

	init(); // Our own custom function.

	glutDisplayFunc(display);
	glutKeyboardFunc(keyDown);
	glutSpecialFunc(keyDownSpec);
	glutKeyboardUpFunc(keyUp);
	glutSpecialUpFunc(keyUpSpec);

	glutMouseFunc(mouseClick);
	glutMotionFunc(mouseMove); // Requires click to register.

	atexit(clean); // This useful GLUT function calls specified function before exiting program.
	glutMainLoop();

	return 0;
}