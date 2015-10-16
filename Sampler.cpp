#include "Sampler.h"

Sampler::Sampler(std::string name) {

	glGenSamplers(1, &id);
	this->name = name;

	//std::cout << "Sampler object constructed\n";
}


Sampler::~Sampler(void) {
	glDeleteSamplers(1, &id);

	//std::cout << "Sampler object destructed\n";
}

void Sampler::bindToUnit(GLuint texture_unit) {

	glBindSampler(texture_unit, id);

}

void Sampler::unbindFromUnit(GLuint texture_unit) {

	glBindSampler(texture_unit, 0);

}