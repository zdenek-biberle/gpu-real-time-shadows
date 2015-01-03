#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <iostream>
#include <fstream>

#include "vertexTypes.h"

struct ModelInfo
{
	ModelInfo();
	ModelInfo(unsigned int baseIndex, unsigned int indexCount);
	ModelInfo(unsigned int baseIndex, unsigned int indexCount, const glm::mat4& transform);
	
	unsigned int baseIndex;
	unsigned int indexCount;
	glm::mat4 transform;
};

static_assert(sizeof(ShadowVolumeVertex) == sizeof(float) * 8, "Velikost struktury ShadowVolumeVertex neni 8 floatu");

// loaduje vertexy, normaly a uvcka
ModelInfo loadModel(
	const std::string& filename, 
	std::vector<Vertex>& vertices, 
	std::vector<GLuint>& indices);

void untangle(std::vector<glm::vec3> &vertices, std::vector<glm::vec3> &normals, std::vector<std::vector<std::string>> &faces, std::vector<Vertex>& outVertices, std::vector<GLuint> &indices);

#endif
