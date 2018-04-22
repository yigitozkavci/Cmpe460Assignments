// OpenGLProject.cpp : Defines the entry point for the console application.
//

// Std. Includes
#include <string>

// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GL includes
/*
#include "Shader.h"
#include "Camera.h"
*/

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Object3D.h"
#include "Shader.h"
#include <vector>

GLFWwindow* window;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

void init(int w, int h)
{
	// Init GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	window = glfwCreateWindow(w, h, "OpenGL Assignment", nullptr, nullptr); // Windowed
	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, key_callback);

	// Initialize GLEW to setup the OpenGL Function pointers
	glewExperimental = GL_TRUE;
	glewInit();

	// Define the viewport dimensions
	glViewport(0, 0, w, h);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
}


int main()
{
	int screenWidth = 800;
	int screenHeight = 600;
	init(screenWidth, screenHeight);
	std::vector<Object3D*> listOfObjects;

	//********This is for example purposes.********
	// You may generate your duplicate objects and modify their model matrices here, procedurally.
	Object3D* pObj = new Object3D();
	//Change with the path to your .ply file
	pObj->CreateObject(
		"C://Users//ufuk.bicici//Documents//Visual Studio 2015//Projects//OpenGLProject//rectangle.ply");
	listOfObjects.push_back(pObj);
	//********This is for example purposes.********

	//Create the shaders. Don't forget to change the paths for your computer.
	Shader shader(
		"C://Users//ufuk.bicici//Documents//Visual Studio 2015//Projects//OpenGLProject//OpenGLProject//VertexShader.vs",
		"C://Users//ufuk.bicici//Documents//Visual Studio 2015//Projects//OpenGLProject//OpenGLProject//FragmentShader.fs");

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		// Clear the colorbuffer
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Use the shader
		shader.Use();
		// Transformations
		// Create camera transformation
		glm::mat4 view;
		glm::vec3 cameraPos = glm::vec3(4.0f, 2.0f, 3.0f);
		glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
		view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
		// Create projection transformation
		glm::mat4 projection;
		projection = glm::perspective<float>(90.0, (float)screenWidth / (float)screenHeight, 0.1f, 1000.0f);
		// Get the uniform locations
		GLint modelLoc = glGetUniformLocation(shader.Program, "model");
		GLint viewLoc = glGetUniformLocation(shader.Program, "view");
		GLint projLoc = glGetUniformLocation(shader.Program, "projection");
		// Pass the view and projection matrices to the shaders
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

		for (auto pObj : listOfObjects)
		{
			glBindVertexArray(pObj->VAO);
			// !!!!! You will need to modify the model matrices of each separate object. "modelMatrix"
			// of Object3D is identity if not modified!!!!!
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(pObj->modelMatrix));
			//glDrawElements(GL_TRIANGLES, object.vertexCount, GL_INT, 0);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}
		// Swap the buffers
		
		glfwSwapBuffers(window);

	}
	for (auto pObj : listOfObjects)
	{
		glDeleteVertexArrays(1, &pObj->VAO);
		glDeleteBuffers(1, &pObj->VBO);
		glDeleteBuffers(1, &pObj->EBO);
		delete pObj;
	}
	glfwTerminate();
	return 0;
}

