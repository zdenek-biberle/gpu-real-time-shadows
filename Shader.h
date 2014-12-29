#pragma once

#include <string>
#include <fstream>
#include <sstream>

#include <GL/glew.h>

#include <vector>
#include <stdexcept>




class Shader
{
public:

	Shader(GLenum type, const std::string &filename);
	~Shader(void);


	bool compile(GLenum type, const std::string &filename);

	bool isCompiled();
	GLuint id;				// ID of shader
	std::string FindFileOrThrow(const std::string &strBasename);



private:
	GLenum type;			// GL_VERTEX_SHADER, GL_FRAGMENT_SHADER...
	bool compiled;			// Whether shader was loaded and compiled
};



class ShaderProgram
{
	public:
		ShaderProgram(std::string Name);
		~ShaderProgram();

	bool addShader(Shader* shader);				//ataches shader
	bool linkProgram();							//links and detaches all shaders

	void useProgram();

	GLuint id;
	const std::string name;



private:
	bool linked;		// Whether shader was loaded and compiled

	std::vector<Shader *> shaders;
	void detachShaders();

};
