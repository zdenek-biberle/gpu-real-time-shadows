#include "shaderLoader.h"

#include <stdexcept>

#include "glUtil.h"
#include "readFile.h"

GLuint createShaderFromSource(const std::string& source, GLenum type, const std::string& filename)
{
	auto sourcePtr = (const GLchar*) source.data();
	auto sourceLength = (GLint) source.size();
	
	auto shader = GLCALL(glCreateShader)(type);
	GLCALL(glShaderSource)(shader, 1, &sourcePtr, &sourceLength);
	GLCALL(glCompileShader)(shader);
	
	GLint compileStatus;
	GLCALL(glGetShaderiv)(shader, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus == GL_FALSE)
	{
		GLint infoLogLength;
		GLCALL(glGetShaderiv)(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
		
		auto log = std::vector<GLchar>(infoLogLength);
		GLCALL(glGetShaderInfoLog)(shader, infoLogLength, nullptr, log.data());
			
		auto exStr = "Kompilace shaderu " + filename + " failnula: \n";
		exStr.append(log.begin(), log.end());
		
		throw std::runtime_error(exStr);
	}
	
	return shader;
}

GLuint createProgram(
	const std::vector<std::string>& vertexShaders, 
	const std::vector<std::string>& geometryShaders,
	const std::vector<std::string>& fragmentShaders)
{
	std::vector<GLuint> shaders;
	try
	{
		for (auto& source : vertexShaders)
			shaders.push_back(createShaderFromSource(source, GL_VERTEX_SHADER));
			
		for (auto& source : geometryShaders)
			shaders.push_back(createShaderFromSource(source, GL_GEOMETRY_SHADER));
			
		for (auto& source : fragmentShaders)
			shaders.push_back(createShaderFromSource(source, GL_FRAGMENT_SHADER));
	
		auto program = GLCALL(glCreateProgram)();
		for (auto shader : shaders)
			GLCALL(glAttachShader)(program, shader);
		
		GLCALL(glLinkProgram)(program);
		GLint linkStatus;
		GLCALL(glGetProgramiv)(program, GL_LINK_STATUS, &linkStatus);
		if (linkStatus == GL_FALSE)
		{
			GLint infoLogLength;
			GLCALL(glGetProgramiv)(program, GL_INFO_LOG_LENGTH, &infoLogLength);
			
			auto log = std::vector<GLchar>(infoLogLength);
			GLCALL(glGetProgramInfoLog)(program, infoLogLength, nullptr, log.data());
			std::string exStr = "Linkování programu selhalo: \n";
			exStr.append(log.begin(), log.end());
			throw std::runtime_error(exStr);
		}
		
		for (auto shader : shaders)
			GLCALL(glDeleteShader)(shader);
			
		return program;
	}
	catch (std::exception& ex)
	{
		for (auto shader : shaders)
			GLCALL(glDeleteShader)(shader);
			
		throw;
	}
}

GLuint createTextureProgram(const std::string& textureShaderSource)
{
	static std::string headersSource = readFile("glsl/textureCore/headers.glsl");
	
	auto modifiedTextureShaderSource = "#version 330\n" + headersSource + "#line 1\n" + textureShaderSource;
	
	std::vector<std::string> vertexShaders;
	std::vector<std::string> geometryShaders;
	std::vector<std::string> fragmentShaders;
	
	vertexShaders.push_back(readFile("glsl/shared/vert.glsl"));
	vertexShaders.push_back(readFile("glsl/textureCore/vert.glsl"));
	
	fragmentShaders.push_back(readFile("glsl/textureCore/noise2D.glsl"));
	fragmentShaders.push_back(readFile("glsl/textureCore/noise3D.glsl"));
	fragmentShaders.push_back(readFile("glsl/textureCore/noise4D.glsl"));
	fragmentShaders.push_back(readFile("glsl/textureCore/noiseFuncs.glsl"));
	fragmentShaders.push_back(readFile("glsl/textureCore/rotationMatrix.glsl"));
	fragmentShaders.push_back(readFile("glsl/textureCore/positiveTrig.glsl"));
	fragmentShaders.push_back(readFile("glsl/textureCore/rand.glsl"));
	fragmentShaders.push_back(readFile("glsl/textureCore/linearsmoothstep.glsl"));
	fragmentShaders.push_back(readFile("glsl/textureCore/frag.glsl"));
	fragmentShaders.push_back(modifiedTextureShaderSource);
	
	return createProgram(vertexShaders, geometryShaders, fragmentShaders);
}
