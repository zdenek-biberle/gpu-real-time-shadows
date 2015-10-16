#pragma once


//#include <glm/glm.hpp>    //uint
#include <GL/glew.h> //GLuint
#include <glutil/glutil.h>
#include <iostream>
#include <memory>
#include "Sampler.h"
#include <fstream>
#include "queryBuffer.h"





class dynamicText;    //because of problem with circular includes
class staticText;	  //Control often uses and congregates a lot of elements which in turn sometimes need 
class Font;			  //this class for window dimensions etc.
class ShaderProgram;
//class Sampler;

//TODO - big problem with circular dependencies. figure that out..


//#include "queryBuffer.h"

using namespace glm;

class Control {

public:
	
	~Control(void);
	

	float windowWidth;
	float windowHeight;
	float aspectRatio;
	unsigned int frame_number;  //not in fps????????
	
	int state;
	
	//bool pause;
	bool stats;			//whether to display statistics - fps checks this - 
	//bool help;			//whether to display help

	

	GLuint globalUniformBlockIndex;
	GLuint modelToWorldMatrixUniform;



	ShaderProgram *addProgram(std::string name);
	ShaderProgram *getProgram(std::string name);
	Sampler *getSampler(std::string name);
	Sampler *addSampler(std::string name);

	std::unique_ptr<Font> font;


	std::unique_ptr<bufferedQuery> timestampQuery;
	//std::unique_ptr<bufferedQuery> timeElapsedQuery;




	glutil::MatrixStack perspectiveMatrix;
	glutil::MatrixStack orthographicMatrix;

	

	static Control* getInstance(){ 

		if(instance == nullptr){ 
			instance = new Control();
		}

		return instance;
	}

	
private:
	Control(void);
	static Control* instance;
	std::vector<std::unique_ptr<ShaderProgram>> programs;
	std::vector<std::unique_ptr<Sampler>> samplers;


};

