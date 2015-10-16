#include "Texture.h"



//////////////////////////////////
/////TEXTURE//////////
///////////////
///////
////
//


Texture::Texture(void) {//add dimension as parameter

	// Generate an OpenGL texture ID for this texture
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);  //this texture is forever 2D

	width = 0;
	height = 0;
	/*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0);

	//glSamplerParameteri(control->font->sampler->id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  //want to have sharp font
	//glSamplerParameteri(control->font->sampler->id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);
	*/
}


Texture::~Texture(void){
	glDeleteTextures(1, &id);
}


/*
till only PNG
data_format determines the composition of each element in data.
internal_format - what will be used in program

TODO - determine specific data_format from the loaded file and dont use parameter for it.. ok.. now it is partially ignored
*/
bool Texture::loadFromFile(std::string filename, GLint data_format, GLint internal_format){
	using namespace glimg;
	ImageSet *imageSet;

	try {

		imageSet = glimg::loaders::stb::LoadFromFile(filename);
  
	} catch(glimg::loaders::stb::StbLoaderException &e) {
		std::cout << e.what() << std::endl;
		return false;
	}

	SingleImage image = imageSet->GetImage(0); //get image at mipmap level 0

	Dimensions dim = image.GetDimensions();
	ImageFormat format = image.GetFormat();

	bool expected_format = true;

	//want only 8bit 4 channel RGBA
	expected_format &= (format.Depth() == BD_PER_COMP_8);
	
	expected_format &= (format.Order() == ORDER_RGBA);

	expected_format &= (format.Components() == FMT_COLOR_RGBA || format.Components() == FMT_COLOR_RGB);

	expected_format &= (format.Type() == DT_NORM_UNSIGNED_INTEGER);

	if (format.Components() == FMT_COLOR_RGBA)
		data_format = GL_RGBA;
	else if (format.Components() == FMT_COLOR_RGB)
		data_format = GL_RGB;

	const GLvoid * data = image.GetImageData();


	if(expected_format){
		glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, id);  

			glTexImage2D(GL_TEXTURE_2D, 0, internal_format, dim.width, dim.height, 0, data_format, GL_UNSIGNED_BYTE, data);

			width = dim.width;
			height = dim.height;

		glBindTexture(GL_TEXTURE_2D, 0);  

	} else return false;

return true;
}

/*
Other than default binding slot can be used.
Not really useful I guess..
*/
void Texture::doMipmaps(GLuint textureSlotID){


	glActiveTexture(GL_TEXTURE0 + textureSlotID);
	glBindTexture(GL_TEXTURE_2D, id);

		glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);

}


