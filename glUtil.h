#ifndef GLUTIL_H
#define GLUTIL_H

#include <sstream>
#include <stdexcept>

#include <GL/glew.h>
#include <GL/gl.h>

void GLAPIENTRY debugFunc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const GLvoid* userParam);
const char* glErrorToString(GLenum err);

template<typename F>
struct GlCallProxy
{
private:	
	const char* fName;
	F f;

public:
	GlCallProxy(F f, const char* fName)
	{
		this->f = f;
		this->fName = fName;
	}

	~GlCallProxy()
	{
		GLenum err = glGetError();
		if (err != GL_NO_ERROR)
		{
			std::ostringstream stream;
			stream << "OGL error " << glErrorToString(err) << " při volání fce " << fName;
			throw std::runtime_error(stream.str());
		}
	}
	
	template<typename... Args>
	auto operator()(Args&&... args) -> decltype(f(args...))
	{
		return f(args...);
	}


};

template<typename F>
GlCallProxy<F> makeGlCallProxy(F f, const char* fName)
{
	return GlCallProxy<F>(f, fName);
}

#define GLCALL(f) (makeGlCallProxy(f, #f))

#endif
