#pragma once

#include <GL/glew.h>


/*
Clases for VAO and VBO to help with forgetting to delete them..
*/

class VAO
{
public:
	VAO(){
		glGenVertexArrays(1, &id);
	};
	~VAO(){
		glDeleteVertexArrays(1, &id);
	};


	GLuint id;

	void bind(){
		glBindVertexArray(id);
	};

	void unbind(){
		glBindVertexArray(0);
	};

};

//remembers type of last binding for unbinding so that's nice
class VBO
{
public:

	VBO(){
		glGenBuffers(1, &id);
	};

	~VBO(){
		glDeleteBuffers(1, &id);
	};


	GLuint id;
	GLuint type;

	void bind(GLuint type = GL_ARRAY_BUFFER){
		this->type = type;
		glBindBuffer(type, id);
	};

	void unbind(){
		glBindBuffer(type, 0);
	};


};

//https://www.packtpub.com/books/content/opengl-40-using-uniform-blocks-and-uniform-buffer-objects
class UBO {
public:

	UBO() {
	
	};
	~UBO();

	GLuint id;

	void bind() {
		glBindBuffer(GL_UNIFORM_BUFFER, id);

	};

	void unbind() {
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	};

};


/*
class SSBO {
	//find limits and keep track of it in static vars..


	//map of program id - ssbo block index??

	GLuint block_index = 0;
	block_index = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, "shader_data");


	void bindToPoint(GLuint ssbo_binding_point_index = 0) {
		glShaderStorageBlockBinding(program, block_index, ssbo_binding_point_index);
	}

	glShaderStorageBlockBinding(program, block_index, 80);

	GLuint binding_point_index = 80;
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_point_index, ssbo);
};*/
/*
glShaderStorageBlockBinding, changes the active shader storage block with an assigned index of storageBlockIndex​ in program object program​.storageBlockIndex​ must be an active shader storage block index in program​.storageBlockBinding​ must be less than the value of GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS.If successful, glShaderStorageBinding specifies that program​ will use the data store of the buffer object bound to the binding point storageBlockBinding​ to read and write the values of the buffer variables in the shader storage block identified by storageBlockIndex​.

Errors
GL_INVALID_VALUE is generated if attribindex​ is greater than or equal to the value of GL_MAX_VERTEX_ATTRIBS.

GL_INVAILD_VALUE is generated if bindingindex​ is greater than or equal to the value of GL_MAX_VERTEX_ATTRIB_BINDINGS.

GL_INVALID_OPERATION is generated if no vertex array object is bound.
*/


/*
//Get the index of the uniform block using glGetUniformBlockIndex.
GLuint blockIndex = glGetUniformBlockIndex(programHandle,"BlobSettings");

//Allocate space for the buffer to contain the data for the uniform block.We get the size of the block using glGetActiveUniformBlockiv.
GLint blockSize;

glGetActiveUniformBlockiv(programHandle, blockIndex,GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);

GLubyte * blockBuffer = (GLubyte *) malloc(blockSize);

//Query for the offset of each variable within the block.To do so, we first find the index of each variable within the block.
// Query for the offsets of each block variable
const GLchar *names[] = { "InnerColor", "OuterColor","RadiusInner", "RadiusOuter" };

GLuint indices[4];
glGetUniformIndices(programHandle, 4, names, indices);

GLint offset[4];
glGetActiveUniformsiv(programHandle, 4, indices,GL_UNIFORM_OFFSET, offset);

//Place the data into the buffer at the appropriate offsets.
GLfloat outerColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
GLfloat innerColor[] = { 1.0f, 1.0f, 0.75f, 1.0f };
GLfloat innerRadius = 0.25f, outerRadius = 0.45f;

memcpy(blockBuffer + offset[0], innerColor,	4 * sizeof(GLfloat));
memcpy(blockBuffer + offset[1], outerColor,	4 * sizeof(GLfloat));
memcpy(blockBuffer + offset[2], &innerRadius,	sizeof(GLfloat));
memcpy(blockBuffer + offset[3], &outerRadius,	sizeof(GLfloat));

//Create the OpenGL buffer object and copy the data into it.
GLuint uboHandle;
glGenBuffers(1, &uboHandle);
glBindBuffer(GL_UNIFORM_BUFFER, uboHandle);
glBufferData(GL_UNIFORM_BUFFER, blockSize, blockBuffer,	GL_DYNAMIC_DRAW);

//Bind the buffer object to the uniform block.
glBindBufferBase(GL_UNIFORM_BUFFER, blockIndex, uboHandle);
*/


/*
hmm.. takze staci mit GL_UNIFORM_BUFFER VBO a navic to privazat k bloku
*/