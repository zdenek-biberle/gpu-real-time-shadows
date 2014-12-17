#include <algorithm>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <limits>
#include <type_traits>

#include <GL/glew.h>
#include <GL/gl.h>

#include <SDL2/SDL.h>

#include <assimp/scene.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "glUtil.h"
#include "readFile.h"
#include "modelLoader.h"
#include "shaderLoader.h"


int main(int argc, char** argv)
{
	if (argc < 3)
	{
		throw std::runtime_error("Nesprávný počet parametrů. Správné použité je: \npgpu-real-time-shadows <model scény> <stínící model>");
	}
	
	auto environmentModelFilename = std::string(argv[1]);
	auto shadowModelFilename = std::string(argv[2]);
	
	SDL_Init(SDL_INIT_VIDEO);
	
	int windowWidth = 1024;
	int windowHeight = 768; 
	
	auto window = SDL_CreateWindow(
		"GPU real time shadows", 
		SDL_WINDOWPOS_UNDEFINED, 
		SDL_WINDOWPOS_UNDEFINED, 
		windowWidth, windowHeight, 
		SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
		
	auto glCtx = SDL_GL_CreateContext(window);
	
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		// glew nejede
		std::ostringstream stream;
		stream << "GLEW error: " << glewGetErrorString(err);
		throw std::runtime_error(stream.str());
	}
	std::cout << "Jedeme: " << glewGetString(GLEW_VERSION) << std::endl;
	
	std::cout << "Loadujeme modely" << std::endl;
	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;
	
	auto environmentModel = loadModel(environmentModelFilename, vertices, indices);
	auto shadowModel = loadModel(shadowModelFilename, vertices, indices);
	
	std::vector<decltype(environmentModel)> scene = {environmentModel, shadowModel};
	
	std::cout << "Vytváříme buffery" << std::endl;
	
	GLuint vbo;
	GLuint ibo;
	GLCALL(glGenBuffers)(1, &vbo);
	GLCALL(glGenBuffers)(1, &ibo);

	GLCALL(glBindBuffer)(GL_ARRAY_BUFFER, vbo);
	GLCALL(glBindBuffer)(GL_ELEMENT_ARRAY_BUFFER, ibo);
	GLCALL(glBufferData)(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	GLCALL(glBufferData)(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
	
	std::cout << "Vstupujeme do hlavní smyčky" << std::endl;
	
	float modelRoty = 0.0f;
	float roty = 0.0f;
	float rotx = 1.0f;
	float dist = -3.0f;
	
	bool rotate = true;
	bool loadShaders = true;
	
	auto ticks = SDL_GetTicks();
	auto ticksDelta = 0;
	
	GLuint baseProgram = 0;
	
	auto run = true;
	while (run)
	{	
		SDL_Event event;
		while (SDL_PollEvent(&event)) 
		{
			 switch (event.type)
			 {
				 case SDL_QUIT:
					run = false;
					break;
					
				case SDL_WINDOWEVENT:
					switch (event.window.event)
					{
						case SDL_WINDOWEVENT_RESIZED:
							windowWidth = event.window.data1;
							windowHeight = event.window.data2;
							GLCALL(glViewport)(0, 0, windowWidth, windowHeight);
							break;
						default:
							break;
					}
					break;
					
				case SDL_MOUSEMOTION:
					if (event.motion.state & SDL_BUTTON_LMASK)
					{
						roty += event.motion.xrel * 0.01f;
						rotx += event.motion.yrel * 0.01f;
					}
					
					if (event.motion.state & SDL_BUTTON_RMASK)
					{
						
					}
					break;
					
				case SDL_MOUSEWHEEL:
					dist += event.wheel.y * 0.1f;
					break;
					
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_r: rotate = !rotate; break;
						
						case SDLK_F5: loadShaders = true; break;
								
					}
					break;
				
				default: 
					break;
			 }
		}
		
		auto lastTicks = ticks;
		ticks = SDL_GetTicks();
		ticksDelta = ticks - lastTicks;
		
		modelRoty += rotate ? 0.0002 * (ticksDelta) : 0.0;
		
		try
		{	
			GLCALL(glEnable)(GL_DEPTH_TEST);	
			GLCALL(glLineWidth)(2.0f);
			GLCALL(glClear)(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
			auto projection = glm::perspective(90.0f, float(windowWidth) / float(windowHeight), 0.1f, 100.0f );
			auto view = glm::rotate(
				glm::rotate(
					glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, dist)),
					rotx,
					glm::vec3(1.0f, 0.0f, 0.0f)
				),
				roty, glm::vec3(0.0f, 1.0f, 0.0f)
			);
			auto model = glm::rotate(
				glm::mat4(1.0f),
				modelRoty, glm::vec3(0.0f, 1.0f, 0.0f)
			);
		
			// loadneme shadery, pokud je to treba
			if (loadShaders)
			{
				std::cout << "Nahráváme shadery" << std::endl;
				loadShaders = false;
				
				if (baseProgram != 0)					
				{
					GLCALL(glDeleteProgram)(baseProgram);
					baseProgram = 0;
				}
				
				try
				{
					std::vector<std::string> vertexShaders;
					std::vector<std::string> geometryShaders;
					std::vector<std::string> fragmentShaders;
					
					vertexShaders.push_back(readFile("glsl/scene/vert.glsl"));
					fragmentShaders.push_back(readFile("glsl/scene/frag.glsl"));
					
					baseProgram = createProgram(vertexShaders, geometryShaders, fragmentShaders);
				}
				catch (std::exception& ex)
				{
					std::cerr << ex.what() << std::endl;
				}
			}
			
			if (baseProgram != 0)
			{
				GLCALL(glUseProgram)(baseProgram);
			
				auto mvLocation = GLCALL(glGetUniformLocation)(baseProgram, "mvMat");
				auto mvNormLocation = GLCALL(glGetUniformLocation)(baseProgram, "mvNormMat");
				auto pLocation = GLCALL(glGetUniformLocation)(baseProgram, "pMat");
				
				auto mvMat = view * model;
				auto mvNormMat = glm::transpose(glm::inverse(glm::mat3(mvMat)));
				auto pMat = projection;
				
				GLCALL(glUniformMatrix4fv)(mvLocation, 1, GL_FALSE, glm::value_ptr(mvMat));
				GLCALL(glUniformMatrix3fv)(mvNormLocation, 1, GL_FALSE, glm::value_ptr(mvNormMat));
				GLCALL(glUniformMatrix4fv)(pLocation, 1, GL_FALSE, glm::value_ptr(pMat));
			
				auto lightDirLocation = GLCALL(glGetUniformLocation)(baseProgram, "lightDir");
				if (lightDirLocation != -1)
				{
					auto lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
					lightDir = glm::mat3(view) * lightDir;
					GLCALL(glUniform3fv)(lightDirLocation, 1, glm::value_ptr(lightDir));
				}
			
				auto numArrays = 3;
			
				for (int i = 0; i < numArrays; i++)
					GLCALL(glEnableVertexAttribArray)(i);
					
				// pozice
				GLCALL(glVertexAttribPointer)(0u, 3, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _x)));
				// texcoord
				GLCALL(glVertexAttribPointer)(1u, 2, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _u))); 
				// normala
				GLCALL(glVertexAttribPointer)(2u, 3, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _nx)));
				
				for (auto modelInfo : scene) {
					GLCALL(glDrawElements)(GL_TRIANGLES, (GLsizei) modelInfo.indexCount, GL_UNSIGNED_INT, reinterpret_cast<void*>(modelInfo.baseIndex * sizeof(GLuint)));
				}
				
				for (int i = 0; i < numArrays; i++)
					GLCALL(glDisableVertexAttribArray)(i);
					
				GLCALL(glUseProgram)(0);
			}
		
			SDL_GL_SwapWindow(window);
		}
		catch (std::exception& ex)
		{
			std::cerr << ex.what() << std::endl;
		}
	}
	
	GLCALL(glDeleteBuffers)(1, &vbo);
	GLCALL(glDeleteBuffers)(1, &ibo);
	SDL_GL_DeleteContext(glCtx);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
