#ifndef SHADERLOADER_H
#define SHADERLOADER_H

#include <string>
#include <vector>

#include <GL/glew.h>
#include <GL/gl.h>

GLuint createShaderFromSource(const std::string& source, GLenum type);
GLuint createProgram(
	const std::vector<std::string>& vertexShaders, 
	const std::vector<std::string>& geometryShaders,
	const std::vector<std::string>& fragmentShaders,
	const std::vector<std::string>& computeShaders);

#endif
