
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
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

#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "glUtil.h"
#include "readFile.h"
#include "modelLoader.h"
#include "shaderLoader.h"
#include "Shader.h"
#include "simplifyModel.h"

struct ShadowVolumeComputationInfo
{
	GLuint triCount;
};

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		throw std::runtime_error("Nesprávný počet parametrů. Správné použité je: \npgpu-real-time-shadows <model scény> <stínící model>");
		return 1; //jinak to pak stejne cte nullptr
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
		SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL/* | SDL_WINDOW_RESIZABLE*/);  //dont know how to resize framebuffer texture right now..

	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);	//must be enabled to allow debug output callback
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	
	auto glCtx = SDL_GL_CreateContext(window);
	
	//glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		// glew nejede
		std::ostringstream stream;
		stream << "GLEW error: " << glewGetErrorString(err);
		throw std::runtime_error(stream.str());
	}
	std::cout << "Jedeme glew: " << glewGetString(GLEW_VERSION) << std::endl;
	
	if (glDebugMessageCallback)
	{	
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(debugFunc, nullptr);
		GLuint unusedIds = 0;
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unusedIds, true);
	}
	else
	{
		std::cerr << "Není k dispozici debug output" << std::endl;
	}
	
	std::cout << "Loadujeme modely" << std::endl;
	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;

	auto environmentModel = loadModel(environmentModelFilename, vertices, indices);
	auto shadowModel = loadModel(shadowModelFilename, vertices, indices);

	std::cout << "Zjednodušujeme stínící model" << std::endl;
	
	std::vector<SimpleVertex> simplifiedVertices;
	std::vector<GLuint> simplifiedIndices;
	auto simplifiedModel = simplifyModel(shadowModel, vertices, indices, simplifiedVertices, simplifiedIndices);

	std::vector<decltype(environmentModel)> scene = { environmentModel/*, shadowModel*/ };

	std::cout << "Vytváříme buffery" << std::endl;

	GLuint shadowVolumeVerticesCount;
	GLuint stencilFrameBufferID;
	GLuint stencilTextureID;

	ShaderProgram simpleProgram("simple");
	ShaderProgram stencilProgram("stencil");
	ShaderProgram lightingProgram("lighting");

	{
		Shader Vshader(GL_VERTEX_SHADER, "./glsl/scene/simple.vert");
		Shader Fshader(GL_FRAGMENT_SHADER, "./glsl/scene/simple.frag");
	
		simpleProgram.addShader(&Vshader);
		simpleProgram.addShader(&Fshader);

		if (!simpleProgram.linkProgram()){
			std::cin.ignore();
			exit(1);
		}
	}

	{
		Shader Vshader(GL_VERTEX_SHADER, "./glsl/scene/stencil.vert");
		Shader Fshader(GL_FRAGMENT_SHADER, "./glsl/scene/stencil.frag");

		stencilProgram.addShader(&Vshader);
		stencilProgram.addShader(&Fshader);

		if (!stencilProgram.linkProgram()){
			std::cin.ignore();
			exit(1);
		}
	}

	{
		Shader Vshader(GL_VERTEX_SHADER, "./glsl/scene/vert.glsl");
		Shader Fshader(GL_FRAGMENT_SHADER, "./glsl/scene/frag.glsl");

		lightingProgram.addShader(&Vshader);
		lightingProgram.addShader(&Fshader);

		if (!lightingProgram.linkProgram()){
			std::cin.ignore();
			exit(1);
		}
	}
	

	//http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/

		// The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.

	//	glGenFramebuffers(1, &stencilFrameBufferID);
	//	glBindFramebuffer(GL_FRAMEBUFFER, stencilFrameBufferID);

		glGenTextures(1, &stencilTextureID);
		glBindTexture(GL_TEXTURE_2D, stencilTextureID);
		{
			std::vector<GLint> tmp;
			tmp.resize(200000, 1);		//3 barvy nefunguji? a to zas proc?
			tmp.resize(400000, 0);
			tmp.resize(windowWidth * windowHeight, -1);	//jen tahle posledne nastavena
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, windowWidth, windowHeight, 0, GL_RED_INTEGER, GL_INT, tmp.data());
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);


		//http://stackoverflow.com/questions/21562103/not-being-able-to-render-into-fbo-with-depth-attachment
		//so using this is supposedly bad idea.. only possible with color attachment??? is this true?

		/*
		// Set "renderedTexture" as our colour attachement #0
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, stencilTextureID, 0);

		// Set the list of draw buffers.
		GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

		
		// Always check that our framebuffer is ok
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
			std::cout << "framebuffer fail\n";
			std::cin.ignore();
			return 1;
		}
		*/

		glBindTexture(GL_TEXTURE_2D, 0);
		
	GLuint vbo;
	GLuint ibo;
	GLuint shadowVolumeBuffer;
	GLuint shadowVolumeComputationInfo;

		glGenBuffers(1, &vbo);
		GLCALL(glGenBuffers)(1, &ibo);
		GLCALL(glGenBuffers)(1, &shadowVolumeBuffer);
		GLCALL(glGenBuffers)(1, &shadowVolumeComputationInfo);

		GLCALL(glBindBuffer)(GL_ARRAY_BUFFER, vbo);
		GLCALL(glBufferData)(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

		GLCALL(glBindBuffer)(GL_ELEMENT_ARRAY_BUFFER, ibo);
		GLCALL(glBufferData)(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

		GLCALL(glBindBuffer)(GL_SHADER_STORAGE_BUFFER, shadowVolumeBuffer);
		// *7, protože pro každý trojúhelník můžeme potenciálně vygenerovat až 7 dalších trojúhelníků
		GLCALL(glBufferData)(GL_SHADER_STORAGE_BUFFER, shadowModel.indexCount * sizeof(ShadowVolumeVertex) * 7, nullptr, GL_DYNAMIC_COPY);

		GLCALL(glBindBuffer)(GL_SHADER_STORAGE_BUFFER, shadowVolumeComputationInfo);

		GLCALL(glBindBuffer)(GL_ARRAY_BUFFER, 0);
		GLCALL(glBindBuffer)(GL_ELEMENT_ARRAY_BUFFER, 0);
		GLCALL(glBindBuffer)(GL_SHADER_STORAGE_BUFFER, 0);

	std::cout << "Generujeme pomocné buffery pro výpočet shadow volume" << std::endl;
	
	GLuint simpleVbo;
	GLuint simpleIbo;
	
	GLCALL(glGenBuffers)(1, &simpleVbo);
	GLCALL(glBindBuffer)(GL_ARRAY_BUFFER, simpleVbo);
	GLCALL(glBufferData)(GL_ARRAY_BUFFER, simplifiedVertices.size() * sizeof(SimpleVertex), simplifiedVertices.data(), GL_STATIC_DRAW);
	
	GLCALL(glGenBuffers)(1, &simpleIbo);
	GLCALL(glBindBuffer)(GL_ELEMENT_ARRAY_BUFFER, simpleIbo);
	GLCALL(glBufferData)(GL_ELEMENT_ARRAY_BUFFER, simplifiedIndices.size() * sizeof(GLuint), simplifiedIndices.data(), GL_STATIC_DRAW);

	std::cout << "Vstupujeme do hlavní smyčky" << std::endl;
	
	float modelRoty = 0.0f;
	float roty = 0.0f;
	float rotx = 1.0f;
	float dist = -4.0f;
	
	bool rotate = true;
	bool loadShaders = true;
	
	auto ticks = SDL_GetTicks();
	auto ticksDelta = 0;
	
	ShadowVolumeComputationInfo shadowVolumeInfo;
	GLuint volumeComputationProgram = 0;
	GLuint volumeVisualizationProgram = 0;
	
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
						roty += event.motion.xrel * 0.1f;
						rotx += event.motion.yrel * 0.1f;
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
			GLCALL(glDepthFunc)(GL_LEQUAL);
			GLCALL(glLineWidth)(0.1f);		//0.0f-1.0f
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
			
			auto mvMat = view * model;
			auto mvNormMat = glm::transpose(glm::inverse(glm::mat3(mvMat)));
			auto pMat = projection;
		
			// loadneme shadery, pokud je to treba
			if (loadShaders)
			{
				std::cout << "Nahráváme shadery" << std::endl;
				loadShaders = false;
				
								
				if (volumeComputationProgram != 0)
				{
					GLCALL(glDeleteProgram)(volumeComputationProgram);
					volumeComputationProgram = 0;
				}
				
				if (volumeVisualizationProgram != 0)
				{
					GLCALL(glDeleteProgram)(volumeVisualizationProgram);
					volumeVisualizationProgram = 0;
				}
				
				try
				{
					std::vector<std::string> computeShaders;

					computeShaders.push_back(readFile("./glsl/volume-computation/compute.glsl"));
					
					volumeComputationProgram = createProgram(
						std::vector<std::string>(), 
						std::vector<std::string>(), 
						std::vector<std::string>(), 
						computeShaders);
				}
				catch (std::exception& ex)
				{
					std::cerr << "Chyba kompilace programu pro generování shadow volume: " << ex.what() << std::endl;
				}
				
				try
				{
					std::vector<std::string> vertexShaders;
					std::vector<std::string> fragmentShaders;

					vertexShaders.push_back(readFile("./glsl/volume-visualization/vert.glsl"));
					fragmentShaders.push_back(readFile("./glsl/volume-visualization/frag.glsl"));
					
					volumeVisualizationProgram = createProgram(
						vertexShaders, 
						std::vector<std::string>(),
						fragmentShaders, 
						std::vector<std::string>());
				}
				catch (std::exception& ex)
				{
					std::cerr << "Chyba kompilace programu pro zobrazování shadow volume: " << ex.what() << std::endl;
				}
			}
			
			if (volumeComputationProgram != 0)
			{
				GLCALL(glUseProgram)(volumeComputationProgram);
				
				shadowVolumeInfo.triCount = 0;
				
				GLCALL(glMemoryBarrier)(GL_ALL_BARRIER_BITS);
				
				GLCALL(glBindBuffer)(GL_SHADER_STORAGE_BUFFER, shadowVolumeComputationInfo);
				GLCALL(glBufferData)(GL_SHADER_STORAGE_BUFFER, sizeof(ShadowVolumeComputationInfo), nullptr, GL_DYNAMIC_READ);
				GLCALL(glBufferData)(GL_SHADER_STORAGE_BUFFER, sizeof(ShadowVolumeComputationInfo), &shadowVolumeInfo, GL_DYNAMIC_READ);
				GLCALL(glBindBuffer)(GL_SHADER_STORAGE_BUFFER, 0);
				
				auto lightDirLocation = glGetUniformLocation(volumeComputationProgram, "lightDir");

				if (lightDirLocation != -1)
				{
					auto lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
					lightDir = glm::mat3(glm::inverse(model)) * lightDir;
					glUniform3fv(lightDirLocation, 1, glm::value_ptr(lightDir));
				}
				
				GLCALL(glBindBufferBase)(GL_SHADER_STORAGE_BUFFER, 0, simpleVbo);
				GLCALL(glBindBufferBase)(GL_SHADER_STORAGE_BUFFER, 1, simpleIbo);
				GLCALL(glBindBufferBase)(GL_SHADER_STORAGE_BUFFER, 2, shadowVolumeBuffer);
				GLCALL(glBindBufferBase)(GL_SHADER_STORAGE_BUFFER, 3, shadowVolumeComputationInfo);
				
				auto indexOffsetLocation = GLCALL(glGetUniformLocation)(volumeComputationProgram, "indexOffset");
				GLCALL(glUniform1ui)(indexOffsetLocation, simplifiedModel.baseIndex);
				
				auto indexCountLocation = GLCALL(glGetUniformLocation)(volumeComputationProgram, "indexCount");
				GLCALL(glUniform1ui)(indexCountLocation, simplifiedModel.indexCount);
				
				GLCALL(glDispatchCompute)((shadowModel.indexCount / 3 + 127) / 128, 1, 1);
				
				GLCALL(glUseProgram)(0);
				GLCALL(glMemoryBarrier)(GL_ALL_BARRIER_BITS);
				
				for (unsigned i = 0; i < 4; i++)
					GLCALL(glBindBufferBase)(GL_SHADER_STORAGE_BUFFER, i, 0);
				
				
				// debug vypisy
				GLCALL(glBindBuffer)(GL_SHADER_STORAGE_BUFFER, shadowVolumeComputationInfo);
				shadowVolumeInfo = *reinterpret_cast<ShadowVolumeComputationInfo*>(GLCALL(glMapBuffer)(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY));
				GLCALL(glUnmapBuffer)(GL_SHADER_STORAGE_BUFFER);
				GLCALL(glBindBuffer)(GL_SHADER_STORAGE_BUFFER, 0);
				
				/*GLCALL(glBindBuffer)(GL_SHADER_STORAGE_BUFFER, shadowVolumeBuffer);
				auto verts = reinterpret_cast<ShadowVolumeVertex*>(GLCALL(glMapBuffer)(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY));
				for (unsigned i = 0; i < shadowVolumeInfo.triCount  * 3; i++)
					std::cout << verts[i].x << ", " << verts[i].y << ", " << verts[i].z << ": " << verts[i].multiplicity << ", " << verts[i].isCap << std::endl;
				GLCALL(glUnmapBuffer)(GL_SHADER_STORAGE_BUFFER);
				GLCALL(glBindBuffer)(GL_SHADER_STORAGE_BUFFER, 0);*/
				GLCALL(glUseProgram)(0);
			}
			



			//draw scene -> fill depth buffer, (color buffer with ambient values?, and blend it with rest..) - simple shaders
			//now somehow use buffer data generated by compute shader as input   ????? - side caps
			//no culling - front and back is already inside multiplicity parameter
			//draw it and fill stencilTexture at index xy (gl_FragCoord.xy) with multiplicity of that fragment. (which is also input from CS)
			
			

			//culling
			//use that texture when drawing scene again to decide what is in shade
			//use new program, bind texture, draw
			//done




			
			//this bit should clear stencil texture between runs
			GLint clearColor[] = { 0 };
			glBindTexture(GL_TEXTURE_2D, stencilTextureID);
			glClearTexImage(stencilTextureID, 0, GL_RED_INTEGER, GL_INT, nullptr);	//
			glBindTexture(GL_TEXTURE_2D, 0);
			
			/*
			std::vector<GLubyte> emptyData(windowWidth * windowHeight * 4, 0);
			glBindTexture(GL_TEXTURE_2D, stencilTextureID);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, windowWidth, windowHeight, GL_RED_INTEGER, GL_INT, &emptyData[0]);
			*/
			///////////////////////////////////////////////////////////////////////////////////////////////////////
			simpleProgram.useProgram();
		
			//glDepthMask(GL_TRUE);

				auto mvLocation = glGetUniformLocation(simpleProgram.id, "mvMat");
				auto pLocation = glGetUniformLocation(simpleProgram.id, "pMat");

				glUniformMatrix4fv(mvLocation, 1, GL_FALSE, glm::value_ptr(mvMat));
				glUniformMatrix4fv(pLocation, 1, GL_FALSE, glm::value_ptr(pMat));

				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

				auto numArrays = 1;

				for (int i = 0; i < numArrays; i++)			//use vao.. this is silly
					glEnableVertexAttribArray(i);

				// pozice
				glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _x)));
		
				for (auto modelInfo : scene) {
					glDrawElements(GL_TRIANGLES, (GLsizei)modelInfo.indexCount, GL_UNSIGNED_INT, reinterpret_cast<void*>(modelInfo.baseIndex * sizeof(GLuint)));
				}

				for (int i = 0; i < numArrays; i++)
					glDisableVertexAttribArray(i);
				
			

				
				
			///////////////////////////////////////////////////////////////////////////////////////////////////////
			//no need to disable drawing to color or depth buffer here
			stencilProgram.useProgram();
			glDepthMask(GL_FALSE);
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);  //just in case

			glBindBuffer(GL_ARRAY_BUFFER, shadowVolumeBuffer); //bind output of compute shader as array buffer
			
			//glActiveTexture(GL_TEXTURE0 + 0);
			//glBindTexture(GL_TEXTURE_2D, stencilTextureID);

			GLuint imageLoc = glGetUniformLocation(stencilProgram.id, "stencilTexture");
			glUniform1i(imageLoc, 0); 
			glBindImageTexture(0, stencilTextureID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);

				mvLocation = glGetUniformLocation(simpleProgram.id, "mvMat");
				pLocation = glGetUniformLocation(simpleProgram.id, "pMat");

				mvMat = view * model;
				pMat = projection;

				glUniformMatrix4fv(mvLocation, 1, GL_FALSE, glm::value_ptr(mvMat));
				glUniformMatrix4fv(pLocation, 1, GL_FALSE, glm::value_ptr(pMat));


				numArrays = 2;

				for (int i = 0; i < numArrays; i++)
					glEnableVertexAttribArray(i);

				// pozice vcetne w -> 4 floaty
				glVertexAttribPointer(0u, 4, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(ShadowVolumeVertex), reinterpret_cast<void*>(offsetof(ShadowVolumeVertex, x)));
				// multiplicita
				glVertexAttribIPointer(1u, 1, GL_INT, (GLsizei) sizeof(ShadowVolumeVertex), reinterpret_cast<void*>(offsetof(ShadowVolumeVertex, multiplicity)));

				
				glDrawArrays(GL_TRIANGLES, 0, shadowVolumeInfo.triCount * 3);


				for (int i = 0; i < numArrays; i++)
					glDisableVertexAttribArray(i);
					

				

			//glBindTexture(GL_TEXTURE_2D, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
			glUseProgram(0);


			///////////////////////////////////////////////////////////////////////////////////////////////////////
			lightingProgram.useProgram();

			glClear(GL_DEPTH_BUFFER_BIT); 

			glDepthMask(GL_TRUE);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

			//glEnable(GL_CULL_FACE);
			//glCullFace(GL_BACK);

				glActiveTexture(GL_TEXTURE0 + 0);
				glBindTexture(GL_TEXTURE_2D, stencilTextureID);
			

					//draw scene with lighting

					//reuse previously declared variables
					mvLocation = glGetUniformLocation(lightingProgram.id, "mvMat");
					auto mvNormLocation = glGetUniformLocation(lightingProgram.id, "mvNormMat");
					pLocation = glGetUniformLocation(lightingProgram.id, "pMat");

					glUniformMatrix4fv(mvLocation, 1, GL_FALSE, glm::value_ptr(mvMat));
					glUniformMatrix3fv(mvNormLocation, 1, GL_FALSE, glm::value_ptr(mvNormMat));
					glUniformMatrix4fv(pLocation, 1, GL_FALSE, glm::value_ptr(pMat));

					auto lightDirLocation = glGetUniformLocation(lightingProgram.id, "lightDir");

					if (lightDirLocation != -1)
					{
						auto lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
						lightDir = glm::mat3(view) * lightDir;
						glUniform3fv(lightDirLocation, 1, glm::value_ptr(lightDir));
					}

					glBindBuffer(GL_ARRAY_BUFFER, vbo);
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

					numArrays = 2;

					for (int i = 0; i < numArrays; i++)
						glEnableVertexAttribArray(i);

					// pozice
					glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _x)));
					// normala
					glVertexAttribPointer(1u, 3, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _nx)));

					for (auto modelInfo : scene) {
						glDrawElements(GL_TRIANGLES, (GLsizei)modelInfo.indexCount, GL_UNSIGNED_INT, reinterpret_cast<void*>(modelInfo.baseIndex * sizeof(GLuint)));
					}

					for (int i = 0; i < numArrays; i++)
						glDisableVertexAttribArray(i);

			
				glBindTexture(GL_TEXTURE_2D, 0);

			GLCALL(glBindBuffer)(GL_ARRAY_BUFFER, 0);
			GLCALL(glBindBuffer)(GL_ELEMENT_ARRAY_BUFFER, 0);	
			GLCALL(glUseProgram)(0);
	
			if (volumeVisualizationProgram != 0)
			{
				GLCALL(glUseProgram)(volumeVisualizationProgram);
			
				auto mvLocation = GLCALL(glGetUniformLocation)(volumeVisualizationProgram, "mvMat");
				auto mvNormLocation = GLCALL(glGetUniformLocation)(volumeVisualizationProgram, "mvNormMat");
				auto pLocation = GLCALL(glGetUniformLocation)(volumeVisualizationProgram, "pMat");
				
				GLCALL(glUniformMatrix4fv)(mvLocation, 1, GL_FALSE, glm::value_ptr(mvMat));
				GLCALL(glUniformMatrix3fv)(mvNormLocation, 1, GL_FALSE, glm::value_ptr(mvNormMat));
				GLCALL(glUniformMatrix4fv)(pLocation, 1, GL_FALSE, glm::value_ptr(pMat));
			
				GLCALL(glBindBuffer)(GL_ARRAY_BUFFER, shadowVolumeBuffer);
			
				auto numArrays = 2;
			
				for (int i = 0; i < numArrays; i++)
					GLCALL(glEnableVertexAttribArray)(i);
					
				// pozice
				GLCALL(glVertexAttribPointer)(0u, 4, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(ShadowVolumeVertex), reinterpret_cast<void*>(offsetof(ShadowVolumeVertex, x)));
				// multiplicita
				GLCALL(glVertexAttribIPointer)(1u, 1, GL_INT, (GLsizei) sizeof(ShadowVolumeVertex), reinterpret_cast<void*>(offsetof(ShadowVolumeVertex, multiplicity)));
				
				GLCALL(glDrawArrays)(GL_TRIANGLES, 0, shadowVolumeInfo.triCount * 3);
				
				for (int i = 0; i < numArrays; i++)
					GLCALL(glDisableVertexAttribArray)(i);
					
				GLCALL(glBindBuffer)(GL_ARRAY_BUFFER, 0);
				GLCALL(glUseProgram)(0);
			}
		
			SDL_GL_SwapWindow(window);
		}
		catch (std::exception& ex)
		{
			std::cerr << ex.what() << std::endl;
		}
	}
	
	glDeleteTextures(1, &stencilTextureID);
	//glDeleteFramebuffers(1, &stencilFrameBufferID);

	GLCALL(glDeleteBuffers)(1, &vbo);
	GLCALL(glDeleteBuffers)(1, &ibo);
	GLCALL(glDeleteBuffers)(1, &shadowVolumeBuffer);
	GLCALL(glDeleteBuffers)(1, &shadowVolumeComputationInfo);
	SDL_GL_DeleteContext(glCtx);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
