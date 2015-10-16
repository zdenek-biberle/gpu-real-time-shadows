#include "Control.h"
#include "Font.h"

Control* Control::instance = nullptr;




Control::Control(void)
{

	
}


Control::~Control(void)
{
	
}

/*
Creates ShaderProgram object and returns pointer to it.
*/
ShaderProgram * Control::addProgram(std::string name){

	programs.push_back(std::make_unique<ShaderProgram>(name));
	
return programs.back().get();
}


Sampler * Control::addSampler(std::string name){

	samplers.push_back(std::make_unique<Sampler>(name));

return samplers.back().get();
}

/*
Returns pointer to ShaderProgram by name.
unlesss count is in the 1000s map/unordered map is not really faster so that it matters.. or at all?
*/
ShaderProgram *Control::getProgram(std::string name){
	
	for(uint i = 0; i < programs.size(); i++){
		if(programs.at(i)->name == name)
			return programs.at(i).get();	
	}

return nullptr;
}

Sampler *Control::getSampler(std::string name){

	for (uint i = 0; i < samplers.size(); i++){
		if (samplers.at(i)->name == name)
			return samplers.at(i).get();
	}

return nullptr;
}


