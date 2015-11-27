#pragma once

#ifdef _MSC_VER
#include <iso646.h>
#endif



#include <unordered_map>
#include <string>
#include <fstream>
#include <sstream>
#include <GL\glew.h>
#include "Shader.h"
#include <vector>
#include <iostream>
#include <memory>


//http://stackoverflow.com/questions/440144/in-opengl-is-there-a-way-to-get-a-list-of-all-uniforms-attribs-used-by-a-shade
//https://www.opengl.org/wiki/Program_Introspection

class Shader
{
public:
	
	Shader(GLenum type, const std::string &filename);
	~Shader(void);


	bool compile(GLenum type, const std::string &filename);

	bool isCompiled();
	bool recompile();

	GLuint id;				// ID of shader
	std::string FindFileOrThrow(const std::string &strBasename);


private:
	GLenum type;			// GL_VERTEX_SHADER, GL_FRAGMENT_SHADER...
	std::string filename;
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
	std::string translateType(GLenum type);
	std::string translateProperty(GLenum resource_property);
	std::string translateInterface(GLenum resource_property);

};

/*
soo to sum it up..
now i can queryResource.. should be anything.. results are in resource with optional name (depends on present GL_NAME_LENGTH property) and 
other properties in two vectors - label and value are at common values of index

after linking program there is self inspection.. 
category is filled during inspection..

still unresolved issue of blocks..
 - simple blocks
 - nested blocks..

*/

class ShaderProgram
{
	public:
		ShaderProgram(std::string Name);
		~ShaderProgram();						

	Shader *addShader(GLenum type, const std::string &filename);		//creates and ataches shader
	bool linkProgram(bool print_introspection = false);							//links and detaches all shaders

	void useProgram();
	bool recompile();

	GLuint id;
	const std::string name;



	GLint getResource(std::string resource_name, resource_property_enum resource_property = LOCATION);
	//std::unordered_map<std::string, resource> resources;
	std::vector<resource> resources;

	resource queryResource(GLint block_index, GLenum programInterface, std::vector<GLenum> properties = { GL_NAME_LENGTH, GL_LOCATION, GL_TYPE }, GLint bufferSize = -1);
	GLint getResourceCount(GLenum programInterface, GLenum interfaceProperty = GL_ACTIVE_RESOURCES);
	void printResources();

private:
	bool linked;		// Whether shader was loaded and compiled

	std::vector<std::unique_ptr<Shader>> shaders;
	void detachShaders();
	void do_introspection(bool print = false);


}; 
