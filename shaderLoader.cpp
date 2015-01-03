#include "shaderLoader.h"

#include <stdexcept>

#include "glUtil.h"
#include "readFile.h"

const char * shaderTypeToString(GLenum shaderType)
{
	switch (shaderType)
	{
		case GL_VERTEX_SHADER: return "GL_VERTEX_SHADER";
		case GL_GEOMETRY_SHADER: return "GL_GEOMETRY_SHADER";
		case GL_FRAGMENT_SHADER: return "GL_FRAGMENT_SHADER";
		case GL_COMPUTE_SHADER: return "GL_COMPUTE_SHADER";
		default: throw std::runtime_error("Neplatný typ shaderu");
	}
}

GLuint createShaderFromSource(const std::string& source, GLenum type)
{
	auto sourcePtr = (const GLchar*) source.data();
	auto sourceLength = (GLint) source.size();
	
	auto shader = glCreateShader(type);
	if (shader == 0) throw std::runtime_error(std::string("Chyba při vytváření shaderu typu ") + shaderTypeToString(type));
	glShaderSource(shader, 1, &sourcePtr, &sourceLength);
	glCompileShader(shader);
	
	GLint compileStatus;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus == GL_FALSE)
	{
		GLint infoLogLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
		
		auto log = std::vector<GLchar>(infoLogLength);
		glGetShaderInfoLog(shader, infoLogLength, nullptr, log.data());
			
		auto exStr = std::string("Kompilace shaderu ") + shaderTypeToString(type) + " failnula: \n";
		exStr.append(log.begin(), log.end());
		
		throw std::runtime_error(exStr);
	}
	
	return shader;
}

GLuint createProgram(
	const std::vector<std::string>& vertexShaders, 
	const std::vector<std::string>& geometryShaders,
	const std::vector<std::string>& fragmentShaders,
	const std::vector<std::string>& computeShaders)
{
	std::vector<GLuint> shaders;
	shaders.reserve(
		vertexShaders.size() + 
		geometryShaders.size() + 
		fragmentShaders.size() + 
		computeShaders.size());
		
	try
	{
		for (auto& source : vertexShaders)
			shaders.push_back(createShaderFromSource(source, GL_VERTEX_SHADER));
			
		for (auto& source : geometryShaders)
			shaders.push_back(createShaderFromSource(source, GL_GEOMETRY_SHADER));
			
		for (auto& source : fragmentShaders)
			shaders.push_back(createShaderFromSource(source, GL_FRAGMENT_SHADER));
			
		for (auto& source : computeShaders)
			shaders.push_back(createShaderFromSource(source, GL_COMPUTE_SHADER));
	
		auto program = glCreateProgram();
		for (auto shader : shaders)
			glAttachShader(program, shader);
		
		glLinkProgram(program);
		GLint linkStatus;
		glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
		if (linkStatus == GL_FALSE)
		{
			GLint infoLogLength;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
			
			auto log = std::vector<GLchar>(infoLogLength);
			glGetProgramInfoLog(program, infoLogLength, nullptr, log.data());
			std::string exStr = "Linkování programu selhalo: \n";
			exStr.append(log.begin(), log.end());
			throw std::runtime_error(exStr);
		}
		
		for (auto shader : shaders)
			glDeleteShader(shader);
			
		return program;
	}
	catch (std::exception& ex)
	{
		for (auto shader : shaders)
			glDeleteShader(shader);
			
		throw;
	}
}
