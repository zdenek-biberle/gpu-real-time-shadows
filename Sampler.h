#pragma once

#include <GL/glew.h>
#include <string>

class Sampler {
public:
	Sampler(std::string name);
	~Sampler(void);

	GLuint id;
	std::string name;
	void bindToUnit(GLuint texture_unit);
	void unbindFromUnit(GLuint texture_unit);

};
