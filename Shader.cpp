#include "Shader.h"

Shader::Shader(GLenum type, const std::string &filename){

	compile(type, filename);

}

Shader::~Shader(void)
{
	glDeleteShader(id);
}

//fragments from Arcsynthesis.org
std::string Shader::FindFileOrThrow( const std::string &strBasename )
	{
		std::string strFilename = strBasename;
		std::ifstream testFile(strFilename.c_str());
		if(testFile.is_open())
			return strFilename;


		strFilename = strBasename;
		testFile.open(strFilename.c_str());
		if(testFile.is_open())
			return strFilename;

		throw std::runtime_error("Could not find the file " + strBasename);
	}

bool Shader::compile(GLenum type, const std::string &filename)	{


		std::string strFilename = FindFileOrThrow(filename);
		std::ifstream shaderFile(strFilename.c_str());
		std::stringstream shaderData;
		shaderData << shaderFile.rdbuf();
		shaderFile.close();

		const std::string& tmp = shaderData.str();
		
   
		id = glCreateShader(type);

		GLint sourceLength = (GLint)shaderData.str().size();
		const GLchar *pText = static_cast<const GLchar *>(tmp.c_str());

		glShaderSource(id, 1, &pText, &sourceLength);
		glCompileShader(id);


		GLint status;
		glGetShaderiv(id, GL_COMPILE_STATUS, &status);

		if (status == GL_FALSE){

			compiled = false;
			return false;
		}



compiled = true;
return true;
}


bool Shader::isCompiled(){
	return compiled;
}



//////////////////////////////////////////
/////////////SHADER PROGRAM//////////////////////
///////////////////////////////////
////////////////////////
//////////////
/////
//

ShaderProgram::ShaderProgram(std::string Name) : name(Name){

	id = glCreateProgram();
}

ShaderProgram::~ShaderProgram(){

	glDeleteProgram(id);
}


bool ShaderProgram::addShader(Shader* shader)
{

	//should be compiled from Shader constructor
	if(! shader->isCompiled()){

		GLint infoLogLength;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetProgramInfoLog(id, infoLogLength, NULL, strInfoLog);

		fprintf(stderr, "Compile failure: %s\n", strInfoLog);

		delete[] strInfoLog;

		return false;
	}


	shaders.push_back(shader);
	glAttachShader(id, shader->id);

	return true;
}

//Links shaders and detaches them before returning.
bool ShaderProgram::linkProgram() {

	glLinkProgram(id);

	GLint linkStatus;

	glGetProgramiv(id, GL_LINK_STATUS, &linkStatus);

	linked = (linkStatus == GL_TRUE); //huh.. warning C4800: 'GLint' : forcing value to bool 'true' or 'false' (performance warning)


	if (linkStatus == GL_FALSE)	{

		GLint infoLogLength;

						 //GL_INFO_LOG_LENGTH - returns GL_TRUE or GL_FALSE
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &infoLogLength); //get program parameter infoLogLength

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetProgramInfoLog(id, infoLogLength, NULL, strInfoLog);  //get info log

		fprintf(stderr, "Linker failure: %s\n", strInfoLog);

		delete[] strInfoLog;
	}

	detachShaders();

	shaders.clear();

return linked;
}


void ShaderProgram::detachShaders(){

	for(size_t i = 0; i < shaders.size(); i++)
		glDetachShader(id, shaders[i]->id);


}


void ShaderProgram::useProgram() {

	if(linked)
		glUseProgram(id);
}

