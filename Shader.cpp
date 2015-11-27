#include "Shader.h"


Shader::Shader(GLenum type, const std::string &filename){

	this->filename = filename;
	this->type = type;
	compile(type, filename);

}

Shader::~Shader(void)
{
	glDeleteShader(id);
}


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

bool Shader::recompile() {

	std::string strFilename = FindFileOrThrow(filename);
	std::ifstream shaderFile(strFilename.c_str());
	std::stringstream shaderData;
	shaderData << shaderFile.rdbuf();
	shaderFile.close();


	const std::string& tmp = shaderData.str();

	GLint sourceLength = (GLint) shaderData.str().size();
	const GLchar *pText = static_cast<const GLchar *>(tmp.c_str());

	glShaderSource(id, 1, &pText, &sourceLength);
	glCompileShader(id);


	GLint status;
	glGetShaderiv(id, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE) {

		compiled = false;
		return false;
	}



	compiled = true;
	return true;
}


//////////////////////////////////////////	
/////////////SHADER PROGRAM//////////////////////
///////////////////////////////////
////////////////////////
//////////////
/////
//

void resource::print()
{

	std::cout << translateInterface(this->interface) << " " << name << std::endl;
	
	for (int i = 0; i < propertyLabels.size(); i++)	{
		

		if (propertyLabels[i] == GL_BLOCK_INDEX) {
			if (propertyValues[i] != -1)
				std::cout << "\t" << translateProperty(propertyLabels[i]) << " " << propertyValues[i] << std::endl;

			continue;
		}

		if(propertyLabels[i] == GL_NAME_LENGTH)	{
			//std::cout << name << " ";
			continue;
		}

		if (propertyLabels[i] == GL_TYPE) {
			std::cout << "\t" << translateProperty(propertyLabels[i]) << " " << translateType(propertyValues[i]) << std::endl;
			continue;
		}

		std::cout << "\t" << translateProperty(propertyLabels[i]) << " " << propertyValues[i] << std::endl;

	}

	std::cout << "\n";

}

ShaderProgram::ShaderProgram(std::string Name) : name(Name){

	id = glCreateProgram();
}

ShaderProgram::~ShaderProgram(){

	glDeleteProgram(id);
}


Shader *ShaderProgram::addShader(GLenum type, const std::string &filename) {

	std::cout << "Compiling " << filename << std::endl;

	//emplace should eliminate copying
	shaders.emplace_back(std::make_unique<Shader>(type, filename));

	

	if (not shaders.back().get()->isCompiled()) {
		shaders.pop_back();
		return nullptr;
	}

	//shaders.push_back(shader);
	glAttachShader(id, shaders.back().get()->id);

	return shaders.back().get();
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
	
	} else {

		do_introspection();
	

	}

	detachShaders();

	//shaders.clear();

return linked;
}


void ShaderProgram::detachShaders(){

	for(size_t i = 0; i < shaders.size(); i++)
		glDetachShader(id, shaders[i].get()->id);


}


void ShaderProgram::useProgram() {

	if(linked)
		glUseProgram(id);
}

bool ShaderProgram::recompile() {

	

	bool success = true;

	for (auto &shader : shaders) {
	
		success &= shader.get()->recompile();
		glAttachShader(id, shader->id);
	}


	linkProgram();

return success;
}


//ok idea is to use it to retrieve all ids/locations and names for everything so 
//it can be saved in map or similar structure and doesn't have to be queried anymore.
void ShaderProgram::do_introspection(){

	std::cout << "PROGRAM: " << this->name << std::endl << std::endl;

	// https://www.opengl.org/wiki/GLAPI/glGetProgramResource
	//TODO - put it in map

	

	//what about blocks within blocks? can i detect it with this?
	//probably will need to revisit this later..

	
	//std::vector<resource> resources;

	//in uniform blocks
	GLint numBlocks = getResourceCount(GL_UNIFORM_BLOCK);
	
	for (int blockIndex = 0; blockIndex < numBlocks; ++blockIndex){
		

		resources.push_back(queryResource(blockIndex, GL_UNIFORM_BLOCK, { GL_NAME_LENGTH, GL_NUM_ACTIVE_VARIABLES }));

		//std::cout << "BLOCK: " << resources.back().name << std::endl;
		//std::cout << "\nBLOCK VARS: " << std::endl;


		GLint numActiveUnifs = 0;

		numActiveUnifs = resources.back().propertyValues[1];

		if (numActiveUnifs == 0)		//without this it will get inactive too?? or just ignores empty..
			continue;

		resource res = queryResource(blockIndex, GL_UNIFORM_BLOCK, { GL_ACTIVE_VARIABLES }, numActiveUnifs);

		
		for (int unifIx = 0; unifIx < numActiveUnifs; ++unifIx)	{

			resources.push_back(queryResource(res.propertyValues[unifIx], GL_UNIFORM, { GL_NAME_LENGTH, GL_LOCATION,  GL_TYPE }));
			
			//std::cout << resources.back().name << "\tid: " << resources.back().propertyValues[1] << "\ttype: " << translateType(resources.back().propertyValues[2]) << std::endl;
		}
	}

	//std::cout << "\nVARS: " << std::endl;
	
	GLint numUniforms = getResourceCount(GL_UNIFORM);

	for (int resource_index = 0; resource_index < numUniforms; ++resource_index){

		resources.push_back(queryResource(resource_index, GL_UNIFORM, { GL_NAME_LENGTH, GL_LOCATION, GL_TYPE, GL_BLOCK_INDEX }));
	}

	/*for (int i = 0; i < resources.size(); i++){

		//if it belongs to block, skip it
		if (resources[i].propertyValues[3] != -1)
			continue;

		//std::cout << resources[i].name << "\tid: " << resources[i].propertyValues[1] << "\ttype: " << translateType(resources[i].propertyValues[2]) << std::endl;
	}*/

	//std::cout << "\nINPUT: " << std::endl;

	numUniforms = getResourceCount(GL_PROGRAM_INPUT);

	for (int resource_index = 0; resource_index < numUniforms; ++resource_index){

		resources.push_back(queryResource(resource_index, GL_PROGRAM_INPUT, { GL_NAME_LENGTH, GL_LOCATION, GL_TYPE }));
	}


	//this->resources = resources;

	//should print all..
		printResources();
		//std::cout << resources[i].name << "\tid: " << resources[i].propertyValues[1] << "\ttype: " << translateConstant(resources[i].propertyValues[0]) << std::endl;
	

	std::cout << std::endl << std::endl;
}

//implicit interfaceProperty is GL_ACTIVE_RESOURCES
GLint ShaderProgram::getResourceCount(GLenum programInterface, GLenum interfaceProperty){

	GLint numberOfUniforms = 0;
	glGetProgramInterfaceiv(id, programInterface, interfaceProperty, &numberOfUniforms);

	return numberOfUniforms;
}

void ShaderProgram::printResources()
{

	for (int i = 0; i < resources.size(); i++) {
		
		resources[i].print();
	}
}

/*
implicit values for properties are { GL_NAME_LENGTH, GL_LOCATION,  GL_TYPE }
and for bufferSize is -1 which means that size of properties is used - each property returns a single value
GL_NAME_LENGTH causes filling in resource property name too
*/
resource ShaderProgram::queryResource(GLint block_index, GLenum programInterface, std::vector<GLenum> properties, GLint bufferSize){

	resource res;

	if (bufferSize == -1)
		res.propertyValues.resize(properties.size());
	else
		res.propertyValues.resize(bufferSize);

	res.propertyLabels = properties;

	glGetProgramResourceiv(id, programInterface, block_index, properties.size(), properties.data(), properties.size(), NULL, res.propertyValues.data());


	//query name only if positively sure I should
	int i = 0;
	for (; i < properties.size(); i++){
		if (properties[i] == GL_NAME_LENGTH)
			break;
	}

	if (i != properties.size()){
		
		std::vector<char> nameData(res.propertyValues[i]);
		glGetProgramResourceName(id, programInterface, block_index, nameData.size(), NULL, &nameData[0]);
		std::string name(nameData.begin(), nameData.end() - 1);
		res.name = name;

	}
	else
		res.name = "-";


	res.interface = programInterface;

return res;
}







/*
Prints resource. If labels are specified only those are printed.
Generally GL_NAME_LENGTH is ignored and name is printed instead.
*//*
void ShaderProgram::printResources() {
	using namespace std;

	
	for(auto &res : resources){

		for (int i = 0; i < res.second.propertyLabels.size(); i++) {
			if (res.second.propertyLabels[i] == GL_NAME_LENGTH) {
				cout << res.second.name << " ";
				continue;
			}

			if (res.second.propertyLabels[i] == GL_TYPE) {
				cout << translateProperty(res.second.propertyLabels[i]) << " " << translateType(res.second.propertyValues[i]) << endl;
				continue;
			}

			cout << translateProperty(res.second.propertyLabels[i]) << " " << res.second.propertyValues[i] << endl;
		}

	}



}
*/

/*
Is meant to avoid constant querying or saving of locations everywhere.
Through this function can be retrieved resource type and location by resource name.
*/
GLint ShaderProgram::getResource(std::string resource_name, resource_property_enum resource_property) {
	//I want to avoid exception when using at() and don't want to insert not present keys with []
/*
	if (resources.count(resource_name) != 1)
		return -1;

	return resources[resource_name].propertyValues[resource_property];*/

	for (int i = 0; i < resources.size(); i++) {
		
		if(resources[i].name == resource_name) {
			for (int j = 0; j < resources[i].propertyLabels.size(); j++) {
				
				if(resources[i].propertyLabels[j] == resource_property) {
					return resources[i].propertyValues[j];
				}

			}
		}
			
	}

	return -1;
}

//yay regexps https://msdn.microsoft.com/en-us/library/2k3te2cs.aspx
//table from https://www.opengl.org/wiki/GLAPI/glGetProgramResource.
/*
Translates gl enums of gl_type to string
*/
std::string resource::translateType(GLenum type) {

	switch (type) {

	case GL_FLOAT:
		return "float";
	case GL_FLOAT_VEC2:
		return "vec2";
	case GL_FLOAT_VEC3:
		return "vec3";
	case GL_FLOAT_VEC4:
		return "vec4";
	case GL_DOUBLE:
		return "double";
	case GL_DOUBLE_VEC2:
		return "dvec2";
	case GL_DOUBLE_VEC3:
		return "dvec3";
	case GL_DOUBLE_VEC4:
		return "dvec4";
	case GL_INT:
		return "int";
	case GL_INT_VEC2:
		return "ivec2";
	case GL_INT_VEC3:
		return "ivec3";
	case GL_INT_VEC4:
		return "ivec4";
	case GL_UNSIGNED_INT:
		return "unsigned int";
	case GL_UNSIGNED_INT_VEC2:
		return "uvec2";
	case GL_UNSIGNED_INT_VEC3:
		return "uvec3";
	case GL_UNSIGNED_INT_VEC4:
		return "uvec4";
	case GL_BOOL:
		return "bool";
	case GL_BOOL_VEC2:
		return "bvec2";
	case GL_BOOL_VEC3:
		return "bvec3";
	case GL_BOOL_VEC4:
		return "bvec4";
	case GL_FLOAT_MAT2:
		return "mat2";
	case GL_FLOAT_MAT3:
		return "mat3";
	case GL_FLOAT_MAT4:
		return "mat4";
	case GL_FLOAT_MAT2x3:
		return "mat2x3";
	case GL_FLOAT_MAT2x4:
		return "mat2x4";
	case GL_FLOAT_MAT3x2:
		return "mat3x2";
	case GL_FLOAT_MAT3x4:
		return "mat3x4";
	case GL_FLOAT_MAT4x2:
		return "mat4x2";
	case GL_FLOAT_MAT4x3:
		return "mat4x3";
	case GL_DOUBLE_MAT2:
		return "dmat2";
	case GL_DOUBLE_MAT3:
		return "dmat3";
	case GL_DOUBLE_MAT4:
		return "dmat4";
	case GL_DOUBLE_MAT2x3:
		return "dmat2x3";
	case GL_DOUBLE_MAT2x4:
		return "dmat2x4";
	case GL_DOUBLE_MAT3x2:
		return "dmat3x2";
	case GL_DOUBLE_MAT3x4:
		return "dmat3x4";
	case GL_DOUBLE_MAT4x2:
		return "dmat4x2";
	case GL_DOUBLE_MAT4x3:
		return "dmat4x3";
	case GL_SAMPLER_1D:
		return "sampler1D";
	case GL_SAMPLER_2D:
		return "sampler2D";
	case GL_SAMPLER_3D:
		return "sampler3D";
	case GL_SAMPLER_CUBE:
		return "samplerCube";
	case GL_SAMPLER_1D_SHADOW:
		return "sampler1DShadow";
	case GL_SAMPLER_2D_SHADOW:
		return "sampler2DShadow";
	case GL_SAMPLER_1D_ARRAY:
		return "sampler1DArray";
	case GL_SAMPLER_2D_ARRAY:
		return "sampler2DArray";
	case GL_SAMPLER_1D_ARRAY_SHADOW:
		return "sampler1DArrayShadow";
	case GL_SAMPLER_2D_ARRAY_SHADOW:
		return "sampler2DArrayShadow";
	case GL_SAMPLER_2D_MULTISAMPLE:
		return "sampler2DMS";
	case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
		return "sampler2DMSArray";
	case GL_SAMPLER_CUBE_SHADOW:
		return "samplerCubeShadow";
	case GL_SAMPLER_BUFFER:
		return "samplerBuffer";
	case GL_SAMPLER_2D_RECT:
		return "sampler2DRect";
	case GL_SAMPLER_2D_RECT_SHADOW:
		return "sampler2DRectShadow";
	case GL_INT_SAMPLER_1D:
		return "isampler1D";
	case GL_INT_SAMPLER_2D:
		return "isampler2D";
	case GL_INT_SAMPLER_3D:
		return "isampler3D";
	case GL_INT_SAMPLER_CUBE:
		return "isamplerCube";
	case GL_INT_SAMPLER_1D_ARRAY:
		return "isampler1DArray";
	case GL_INT_SAMPLER_2D_ARRAY:
		return "isampler2DArray";
	case GL_INT_SAMPLER_2D_MULTISAMPLE:
		return "isampler2DMS";
	case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
		return "isampler2DMSArray";
	case GL_INT_SAMPLER_BUFFER:
		return "isamplerBuffer";
	case GL_INT_SAMPLER_2D_RECT:
		return "isampler2DRect";
	case GL_UNSIGNED_INT_SAMPLER_1D:
		return "usampler1D";
	case GL_UNSIGNED_INT_SAMPLER_2D:
		return "usampler2D";
	case GL_UNSIGNED_INT_SAMPLER_3D:
		return "usampler3D";
	case GL_UNSIGNED_INT_SAMPLER_CUBE:
		return "usamplerCube";
	case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
		return "usampler2DArray";
	case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
		return "usampler2DArray";
	case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
		return "usampler2DMS";
	case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
		return "usampler2DMSArray";
	case GL_UNSIGNED_INT_SAMPLER_BUFFER:
		return "usamplerBuffer";
	case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
		return "usampler2DRect";
	default: return "unknown constant";
	}
}

/*
Translates gl enums to string
*/
std::string resource::translateProperty(GLenum resource_property) {

	switch (resource_property) {
		case GL_NAME_LENGTH:
			return "NAME_LENGTH";
		case GL_TYPE:
			return "TYPE";
		case GL_ARRAY_SIZE:
			return "ARRAY_SIZE";
		case GL_OFFSET:
			return "OFFSET";
		case GL_BLOCK_INDEX:
			return "BLOCK_INDEX";
		case GL_ARRAY_STRIDE:
			return "ARRAY_STRIDE";
		case GL_MATRIX_STRIDE:
			return "MATRIX_STRIDE";
		case GL_IS_ROW_MAJOR:
			return "IS_ROW_MAJOR";
		case GL_ATOMIC_COUNTER_BUFFER_INDEX:
			return "ATOMIC_COUNTER_BUFFER_INDEX";
		case GL_BUFFER_BINDING:
			return "BUFFER_BINDING";
		case GL_BUFFER_DATA_SIZE:
			return "BUFFER_DATA_SIZE";
		case GL_NUM_ACTIVE_VARIABLES:
			return "NUM_ACTIVE_VARIABLES";
		case GL_ACTIVE_VARIABLES:
			return "ACTIVE_VARIABLES";
		case GL_REFERENCED_BY_VERTEX_SHADER:
			return "REFERENCED_BY_VERTEX_SHADER";
		case GL_REFERENCED_BY_TESS_CONTROL_SHADER:
			return "REFERENCED_BY_TESS_CONTROL_SHADER";
		case GL_REFERENCED_BY_TESS_EVALUATION_SHADER:
			return "REFERENCED_BY_TESS_EVALUATION_SHADER";
		case GL_REFERENCED_BY_GEOMETRY_SHADER:
			return "REFERENCED_BY_GEOMETRY_SHADER";
		case GL_REFERENCED_BY_FRAGMENT_SHADER:
			return "REFERENCED_BY_FRAGMENT_SHADER";
		case GL_REFERENCED_BY_COMPUTE_SHADER:
			return "REFERENCED_BY_COMPUTE_SHADER";
		case GL_NUM_COMPATIBLE_SUBROUTINES:
			return "NUM_COMPATIBLE_SUBROUTINES";
		case GL_COMPATIBLE_SUBROUTINES:
			return "COMPATIBLE_SUBROUTINES";
		case GL_TOP_LEVEL_ARRAY_SIZE:
			return "TOP_LEVEL_ARRAY_SIZE";
		case GL_TOP_LEVEL_ARRAY_STRIDE:
			return "TOP_LEVEL_ARRAY_STRIDE";
		case GL_LOCATION:
			return "LOCATION";
		case GL_LOCATION_INDEX:
			return "LOCATION_INDEX";
		case GL_IS_PER_PATCH:
			return "IS_PER_PATCH";
		/*case GL_LOCATION_COMPONENT:		//undefined ..yet
			return "LOCATION_COMPONENT";
		case GL_TRANSFORM_FEEDBACK_BUFFER_INDEX:
			return "TRANSFORM_FEEDBACK_BUFFER_INDEX";
		case GL_TRANSFORM_FEEDBACK_BUFFER_STRIDE:
			return "TRANSFORM_FEEDBACK_BUFFER_STRIDE";*/
		default:
			return "unknown property name";
	}
}

std::string resource::translateInterface(GLenum resource_property) {
	switch (resource_property) {
		case GL_UNIFORM:
			return "UNIFORM";
		case GL_UNIFORM_BLOCK:
			return "UNIFORM_BLOCK";
		case GL_PROGRAM_INPUT:
			return "PROGRAM_INPUT";
		case GL_PROGRAM_OUTPUT:
			return "PROGRAM_OUTPUT";
		case GL_BUFFER_VARIABLE:
			return "BUFFER_VARIABLE";
		case GL_SHADER_STORAGE_BLOCK:
			return "SHADER_STORAGE_BLOCK";
		case GL_ATOMIC_COUNTER_BUFFER:
			return "ATOMIC_COUNTER_BUFFER";
		case GL_VERTEX_SUBROUTINE:
			return "VERTEX_SUBROUTINE";
		case GL_TESS_CONTROL_SUBROUTINE:
			return "TESS_CONTROL_SUBROUTINE";
		case GL_TESS_EVALUATION_SUBROUTINE:
			return "TESS_EVALUATION_SUBROUTINE";
		case GL_GEOMETRY_SUBROUTINE:
			return "GEOMETRY_SUBROUTINE";
		case GL_FRAGMENT_SUBROUTINE:
			return "FRAGMENT_SUBROUTINE";
		case GL_COMPUTE_SUBROUTINE:
			return "COMPUTE_SUBROUTINE";
		case GL_VERTEX_SUBROUTINE_UNIFORM:
			return "VERTEX_SUBROUTINE_UNIFORM";
		case GL_TESS_CONTROL_SUBROUTINE_UNIFORM:
			return "TESS_CONTROL_SUBROUTINE_UNIFORM";
		case GL_TESS_EVALUATION_SUBROUTINE_UNIFORM:
			return "TESS_EVALUATION_SUBROUTINE_UNIFORM";
		case GL_GEOMETRY_SUBROUTINE_UNIFORM:
			return "GEOMETRY_SUBROUTINE_UNIFORM";
		case GL_FRAGMENT_SUBROUTINE_UNIFORM:
			return "FRAGMENT_SUBROUTINE_UNIFORM";
		case GL_COMPUTE_SUBROUTINE_UNIFORM:
			return "COMPUTE_SUBROUTINE_UNIFORM";
		case GL_TRANSFORM_FEEDBACK_VARYING:
			return "TRANSFORM_FEEDBACK_VARYING";
		default:
			return "unknown interface name";
	}
}