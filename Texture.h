#pragma once

#include <GL/glew.h>
#include <string>

#include <glimg\glimg.h>

#include <iostream>
#include "Sampler.h"


class Texture
{
public:
	Texture(void);
	~Texture(void);


	GLuint id;
	GLuint width;
	GLuint height;

	bool loadFromFile(std::string filename, GLint data_format, GLint internal_format);
	void doMipmaps(GLuint textureSlotID = 0);

};

