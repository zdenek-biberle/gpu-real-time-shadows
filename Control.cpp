#include "Control.h"
#include "Font.h"

Control* Control::instance = nullptr;




/*
Creates ShaderProgram object and returns pointer to it.
*/
ShaderProgram * baseControl::addProgram(std::string name) {

	programs.push_back(std::make_unique<ShaderProgram>(name));

	return programs.back().get();
}


Sampler * baseControl::addSampler(std::string name) {

	samplers.push_back(std::make_unique<Sampler>(name));

	return samplers.back().get();
}

/*
Returns pointer to ShaderProgram by name.
unlesss count is in the 1000s map/unordered map is not really faster so that it matters.. or at all?
*/
ShaderProgram *baseControl::getProgram(std::string name) {

	for (uint i = 0; i < programs.size(); i++) {
		if (programs.at(i)->name == name)
			return programs.at(i).get();
	}

	return nullptr;
}

Sampler *baseControl::getSampler(std::string name) {

	for (uint i = 0; i < samplers.size(); i++) {
		if (samplers.at(i)->name == name)
			return samplers.at(i).get();
	}

	return nullptr;
}



/*
Recomputes projection matrices and saves the new values for width and height.
*/
void baseControl::recomputeProjections(float windowWidth, float windowHeight, float fov) {

	this->windowHeight = windowHeight;
	this->windowWidth = windowWidth;

	perspectiveMatrix = glm::perspective(glm::radians(fov), float(windowWidth) / float(windowHeight), 0.1f, 1000.0f);
	orthographicMatrix = glm::ortho((float) 0, windowWidth, (float) 0, windowHeight);

}

bool Control::recompileProgram(std::string name) {

	auto program = getProgram(name);

	if (program != nullptr)
		return program->recompile();

	return false;
}

bool Control::recompileAllPrograms() {
	bool success = true;

	for (auto &program : programs) {

		success &= program.get()->recompile();

	}

	return success;
}
