#pragma once


#include <GL/glew.h> 
#include <vector>

/*
front is exposed for data input
back is used for drawing - data output

must explicitly swap them after all use
*/
class doubleBuffer
{
public:
	doubleBuffer(GLuint type = GL_ARRAY_BUFFER);
	~doubleBuffer();

	GLuint getFront();
	GLuint getBack();

	void swapBuffers();

	void initBuffer(GLsizeiptr size, const GLvoid *data, GLenum usage = GL_DYNAMIC_DRAW);  //init swaps buffers so that init data is ready to use in back buffer and front is free to receive new data
private:
	// the array to store the sets of queries.
	GLuint type;
	GLuint bufferID[2];
	unsigned int backBuffer;
	unsigned int frontBuffer;



};

