#pragma once

#include <unordered_map>
#include <string>
#include <fstream>
#include <sstream>
#include <GL/glew.h> 
#include "Shader.h"
#include <vector>
#include <iostream>
#define not !

//http://stackoverflow.com/questions/440144/in-opengl-is-there-a-way-to-get-a-list-of-all-uniforms-attribs-used-by-a-shade
//https://www.opengl.org/wiki/Program_Introspection

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

//this enum acts as indices for default queried properties of program
enum resource_property_enum { LOCATION, TYPE, INTERFACE, NAME /*block index and more?*/ };

class resource {
public:
	std::string name = "-";
	std::vector<GLenum> propertyLabels;
	std::vector<GLint> propertyValues;
	GLenum interface = GL_UNIFORM;

	void print();
};

/*
soo to sum it up..
now i can queryResource.. should be anything.. results are in resource with optional name (depends on present GL_NAME_LENGTH property) and 
other properties in two vectors - label and value are at common values of index

after linking program there is self inspection.. 
category is filled during inspection..

still unresolved issue of blocks..

*/

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



	GLint getResource(std::string resource_name, resource_property_enum resource_property = LOCATION);
	//std::unordered_map<std::string, resource> resources;

	resource queryResource(GLint block_index, GLenum programInterface, std::vector<GLenum> properties = { GL_NAME_LENGTH, GL_LOCATION, GL_TYPE }, GLint bufferSize = -1);
	std::string translateType(GLenum type);
	std::string translateProperty(GLenum resource_property);
	GLint getResourceCount(GLenum programInterface, GLenum interfaceProperty = GL_ACTIVE_RESOURCES);
	void printResources();

private:
	bool linked;		// Whether shader was loaded and compiled
	std::vector<resource> resources;

	std::vector<Shader *> shaders;
	void detachShaders();
	void do_introspection();


}; 
