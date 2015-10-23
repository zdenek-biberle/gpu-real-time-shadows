#pragma once


#include <glm/glm.hpp>    //uint
#include <GL/glew.h> //GLuint
#include <glutil/glutil.h>
#include <iostream>
#include <memory>
//#include "Sampler.h"
#include <fstream>
#include "queryBuffer.h"

//class bufferedQuery;
class dynamicText;    //because of problem with circular includes
class staticText;	  //Control often uses and congregates a lot of elements which in turn sometimes need 
class Font;			  //this class for window dimensions etc.
class ShaderProgram;
class Sampler;
//TODO - big problem with circular dependencies. figure that out..



using namespace glm;

/*
Stuff which is universally needed during the whole time program runs 
and useful in many classes and situations.
Owns all programs, samplers and maybe more..
Keeps actual window dimensions and transformation matrices.
*/
class baseControl {

public:

	~baseControl(void);


	float windowWidth;
	float windowHeight;
	unsigned int frame_number = 0;  //not in fps????????

	unsigned int state = 0;

	bool pause = false;
	bool stats = false;			//whether to display statistics - fps checks this - 
	bool help = false;			//whether to display help



	GLuint globalUniformBlockIndex;
	//GLuint modelToWorldMatrixUniform; //???



	ShaderProgram *addProgram(std::string name);
	ShaderProgram *getProgram(std::string name);
	Sampler *getSampler(std::string name);
	Sampler *addSampler(std::string name);

	


	glutil::MatrixStack perspectiveMatrix;
	glutil::MatrixStack orthographicMatrix;


	void recomputeProjections(float windowWidth, float windowHeight, float fov = 90.0f);


private:
	std::vector<std::unique_ptr<ShaderProgram>> programs;
	std::vector<std::unique_ptr<Sampler>> samplers;


};


class Control : public baseControl {

public:
	
	~Control(void) {};


	std::unique_ptr<Font> font;


	//std::unique_ptr<bufferedQuery> timeElapsedQuery;

	
	
	static Control* getInstance(){ 

		if(instance == nullptr){ 
			instance = new Control();
		}

		return instance;
	}
	
	
private:
	//Control(void);
	static Control* instance;
	

};

