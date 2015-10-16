#include "doubleBuffer.h"


doubleBuffer::doubleBuffer(GLuint type)
{
	this->type = type;
	backBuffer = 0, frontBuffer = 1;

	glGenBuffers(1, &bufferID[0]);
	glGenBuffers(1, &bufferID[1]);
}

//init swaps buffers afterwards so that init data is ready to use in back buffer and front is free to receive new data
//no data makes the draw call crash
void doubleBuffer::initBuffer(GLsizeiptr size, const GLvoid *data, GLenum usage){

	glBindBuffer(type, getFront());

	
		glBufferData(type, size, data, usage);

	glBindBuffer(type, 0);


swapBuffers();
}


doubleBuffer::~doubleBuffer()
{
	glDeleteBuffers(1, &bufferID[0]);
	glDeleteBuffers(1, &bufferID[1]);
	
}


void doubleBuffer::swapBuffers() {

	std::swap(backBuffer, frontBuffer);
	
}

GLuint doubleBuffer::getFront(){
	return bufferID[frontBuffer];
}

GLuint doubleBuffer::getBack(){

	return bufferID[backBuffer];
}
