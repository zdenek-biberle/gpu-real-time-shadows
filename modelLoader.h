#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <iostream>
#include <fstream>

struct ModelInfo
{
	unsigned int baseIndex;
	unsigned int indexCount;
};

struct Vertex
{
	Vertex(
		float x, float y, float z, 
		float nx, float ny, float nz, 
		float u, float v);

	Vertex(
		glm::vec3 point,
		glm::vec3 normal,
		glm::vec2 tex);

	float _x;
	float _y;
	float _z;
	
	float _nx;
	float _ny;
	float _nz;
	
	float _u;
	float _v;
};

struct SimpleVertex
{
	SimpleVertex(
		float x, float y, float z);
	SimpleVertex(
		glm::vec3 p);
	float _x;
	float _y;
	float _z;
	float _pad;
};

// loaduje vertexy, normaly a uvcka
ModelInfo loadModel(
	const std::string& filename, 
	std::vector<Vertex>& vertices, 
	std::vector<GLuint>& indices);

void untangle(std::vector<glm::vec3> &vertices, std::vector<glm::vec3> &normals, std::vector<std::vector<std::string>> &faces, std::vector<Vertex>& outVertices, std::vector<GLuint> &indices);

// loaduje jenom vertexy
ModelInfo loadSimpleModel(
	const std::string& filename, 
	std::vector<SimpleVertex>& vertices, 
	std::vector<GLuint>& indices);

#endif
