#include "glUtil.h"

#include <iostream>

void GLAPIENTRY debugFunc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const GLvoid* userParam)
{
	std::string srcName;
	switch(source)
	{
		case GL_DEBUG_SOURCE_API_ARB: srcName = "API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: srcName = "Window System"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: srcName = "Shader Compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: srcName = "Third Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION_ARB: srcName = "Application"; break;
		case GL_DEBUG_SOURCE_OTHER_ARB: srcName = "Other"; break;
	}
	std::string errorType;
	switch(type)
	{
		case GL_DEBUG_TYPE_ERROR_ARB: errorType = "Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: errorType = "Deprecated Functionality"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: errorType = "Undefined Behavior"; break;
		case GL_DEBUG_TYPE_PORTABILITY_ARB: errorType = "Portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE_ARB: errorType = "Performance"; break;
		case GL_DEBUG_TYPE_OTHER_ARB: errorType = "Other"; break;
	}
	std::string typeSeverity;
	switch(severity)
	{
		case GL_DEBUG_SEVERITY_HIGH_ARB: typeSeverity = "High"; break;
		case GL_DEBUG_SEVERITY_MEDIUM_ARB: typeSeverity = "Medium"; break;
		case GL_DEBUG_SEVERITY_LOW_ARB: typeSeverity = "Low"; break;
	}
	
	std::cout << errorType << " in " << srcName << " - " << id << " [" << typeSeverity << "]: " << std::string(message, length) << std::endl;
	//std::cin.ignore(); //nvidia spamí debug vším možným, takže to komentuju
}

const char* glErrorToString(GLenum err)
{
	switch (err)
	{
		case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
		case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
		case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
		case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
		case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
		default: throw std::runtime_error("Invalid GL error code");
	}
}
