#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <string>
#include <vector>

#include <GL/glew.h>
#include <GL/gl.h>

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

// loaduje jenom vertexy
ModelInfo loadSimpleModel(
	const std::string& filename, 
	std::vector<SimpleVertex>& vertices, 
	std::vector<GLuint>& indices);

#endif
