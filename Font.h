#pragma once

#include <GL/glew.h> //GLuint, glFunctions()
#include <sstream>
#include <vector>
#include <memory>
#include <algorithm>
#include <glm/glm.hpp>   //uint
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ft2build.h>	//freetype
#include FT_FREETYPE_H

#include "Control.h"
#include "Texture.h"
#include "Shader.h"


#include "Animation.h"
#include "doubleBuffer.h"
#include "Animated.h"
#include "glObjects.h"

using namespace glm;




/*
Stores character parameters and geometry
+ mapping in texture atlas.
*/
class Character {

public:

	int advanceX, advanceY;
	int bearingX, bearingY;
	uint width, height;

	std::vector<GLfloat> vertexQuad; //sligtly misleading.. stores 2 triangles..
	std::vector<GLfloat> textureQuad; //sligtly misleading.. stores mapping for 2 triangles..

};


/*
Class for loading truetype fonts for use in Text and dynamicText.
*/
class Font {

public:
	Font(void);
	~Font(void);

	
	bool loadFont(std::string file, int size);  //creates font texture, characters and VAO + VBO
	void addProgram(GLuint prog); // is needed for getting location of sampler, colorUniform - used in Text class
	void doMipmaps();

	GLuint UBO;   //for toCamera transform.. currently set always identity

	FT_Library library;
	FT_Face typeface;


	std::vector<Character> characters;   //stores alphabet
	uint newLine;		//size of Y offset for new line

	Sampler *sampler;		//move this to control - same as programs..
	GLuint program;

	bool loaded;
	uint pixelSize;   //is a number used as size parameter when loading font.

	Texture texture;   //character atlas.. contains all glyphs

};



//TODO - still awkward positioning.. look into that..
// - remove that silly thing - not needed anyway now..

//do drawing as is done in rectangle class

/*
Class Text is meant as class for static text to be displayed with Font.
Font must be loaded or nothing will happen.
The message is kept but not used after creation of Text
*/
class Text : public Animated { 
	
	// issue when resized it stays fixed distance from bottom.. - reload function... rebuilds data + upload.., remember x, y, size.... 
	//OR you know just use dynamic text.. no one is forcing you to change it every frame..

public:
	
	Text();

	int msg_length;
	Font *font;
	GLuint colorUniform;

	bool ready();							//check if data/text is prepared for display - that is it's uploaded and UBO was changed from 0 at any point.
	int width;
	int height;
	bool centered;
	vec4 color;
	GLuint transformMatrixID;

	//std::vector<Animation> moveAnimation;  //it's vector to enable adding multiple animations together
	//std::vector<Animation> rotateAnimation;

	vec3 position;		//..
	vec3 rotation;		//..
	
	//is translation and rotation as an arguments good idea?
	//shouldnt animation take care of that?

	//adjusts position by half of width and height
	void positionIsCenter();
	void positionIsTopLeft();

protected:
	void addTextData(const std::string &text, uint windowHeight, 
		uint offset_x, uint offset_y, uint size);			//adds text geometry to data

	


	bool uploaded;							//used in print to prevent unnecessary bother
	std::vector<GLfloat> data;				//temporary store for drawing data. Deleted after upload.

	VAO vao;
	GLuint UBO;								//for toCamera transform.. currently set always identity
	GLuint fontSampler;
};


class staticText : public Text {

public:
	staticText(Font *font, const std::string message, uint position_x, uint position_y, uint size);
	~staticText();

	void initVAO();							//creates VAO + VBO and binds attribute arrays
	VBO vbo;
	void uploadData();						//uploads text geometry from data to VBO + clears data vector
	void print(float scale = 1.0);	//draws with VAO
};



/*
Extends class Text with property modified.
Creation of new text data and its upload to GPU is potentially
lot of work so the modified property helps to undergo this process
only if necessary.
*/
class dynamicText : public Text {  //idea - make use of SubData and some upper limit for message length - prevent constant reallocation?

public:
	dynamicText(Font *font);
	~dynamicText();

	doubleBuffer VBObuffer;
	void print();							//draws with VAO, must bind appropriate VBO and use glAttribPointer
	void printAndSwapBuffers();				//..

	bool modified;
	void uploadData();						//uploads text geometry from data to frontVBO + clears data vector

	/*
	This is to be manually called to construct text data from one or more parts on one or more locations on screen.
	Sets modified to true.
	*/
	void addTextData(const std::string &text, uint windowHeight, uint x, uint y, uint size){	
		//it could remember this stuff, mostly it stays the same..  or can break it down into separate units with
		//own remembered parameters.. can get rewritten or only partially updated
	
		if (uploaded)		//this means we are adding new data so start new count
			msg_length = 0;

		msg_length += text.size();

		Text::addTextData(text, windowHeight,  x,  y,  size);			//adds text geometry to data

		modified = true;
	}

private:
	void initVAO();




};



