#pragma once


#include <GL/glew.h>
#include <string>
#include "GeometricDefinitions.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


class Object3D
{
public:
	GLuint VBO;
	GLuint EBO;
	GLuint VAO;
	GLuint texture;
	int vertexCount;
	int triangleCount;
	glm::mat4 modelMatrix;
	Vertex* vlist;
	Triangle* tlist;

	Object3D();
	~Object3D();

	void CreateObject(const std::string& ply_file_path);
};
