
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
#include "edgeLookup.h"
#include "shadowComputationTypes.h"
#include "cpuImpl.h"

#include "fps_counter.h"
#include "queryBuffer.h"

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
		SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);	//must be enabled to allow debug output callback
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	
	auto glCtx = SDL_GL_CreateContext(window);
	
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
	
	std::cout << "Generujeme buffer pro vyhledávání hran" << std::endl;
	
	std::vector<EdgeLookupNode> edgeLookup;
	generateEdgeLookup(simplifiedModel, simplifiedIndices, edgeLookup);
	
	std::vector<decltype(environmentModel)*> scene = { &environmentModel, &shadowModel };
	

	std::unique_ptr<fps_counter> fps;
	std::unique_ptr<Font> font;
	std::unique_ptr<bufferedQuery> timestampQuery;
	//std::unique_ptr<bufferedQuery> timeElapsedQuery;



	std::cout << "Creating buffers" << std::endl;

	GLuint shadowVolumeVerticesCount;
	GLuint stencilFrameBufferID;
	GLuint stencilTextureID;

	//add programs to control..

	ShaderProgram simpleProgram("simple");
	ShaderProgram stencilProgram("stencil");
	ShaderProgram lightingProgram("lighting");
	ShaderProgram volumeComputationProgram("volumeComputation");
	ShaderProgram volumeVisualizationProgram("volumeVisualization");
	ShaderProgram fontProgram("font");


	{
		Shader Vshader(GL_VERTEX_SHADER, "./glsl/font/FontVS.vert");
		Shader Fshader(GL_FRAGMENT_SHADER, "./glsl/font/FontFS.frag");


		fontProgram.addShader(&Vshader);
		fontProgram.addShader(&Fshader);

		if (! fontProgram.linkProgram()) {
			std::cin.ignore();
			exit(1);
		}

		
	}

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
	

	{
		Shader Cshader(GL_COMPUTE_SHADER, "./glsl/volume-computation/compute.glsl");
		
		volumeComputationProgram.addShader(&Cshader);

		if (!volumeComputationProgram.linkProgram()) {
			std::cin.ignore();
			exit(1);
		}
	}

	
	Shader vertexShaders(GL_VERTEX_SHADER, "./glsl/volume-visualization/vert.glsl");
	Shader fragmentShaders(GL_FRAGMENT_SHADER, "./glsl/volume-visualization/frag.glsl");

	volumeVisualizationProgram.addShader(&vertexShaders);
	volumeVisualizationProgram.addShader(&fragmentShaders);

	if (!volumeVisualizationProgram.linkProgram()) {
		std::cin.ignore();
		exit(1);
	}


	/*
	control->font = std::unique_ptr<Font>(new Font());
	control->font->addProgram(control->getProgram("font")->id);

	//create font sampler
	control->font->sampler = std::unique_ptr<Sampler>(new Sampler());

	glSamplerParameteri(control->font->sampler->id, GL_TEXTURE_MIN_LOD, 0);
	glSamplerParameteri(control->font->sampler->id, GL_TEXTURE_MAX_LOD, 0);

	glSamplerParameteri(control->font->sampler->id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  //want to have sharp font
	glSamplerParameteri(control->font->sampler->id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glSamplerParameteri(control->font->sampler->id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(control->font->sampler->id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);




	control->modelToWorldMatrixUniform = glGetUniformLocation(control->getProgram("maze")->id, "modelToWorldMatrix");
	control->globalUniformBlockIndex = glGetUniformBlockIndex(control->getProgram("maze")->id, "GlobalMatrices");  //perspective, position of camera


	glUniformBlockBinding(control->getProgram("maze")->id, glGetUniformBlockIndex(control->getProgram("maze")->id, "GlobalMatrices"), globalMatricesBindingIndex);
	glUniformBlockBinding(control->getProgram("font")->id, glGetUniformBlockIndex(control->getProgram("font")->id, "GlobalMatrices"), globalMatricesBindingIndex);

	//create uniform buffer
	glGenBuffers(1, &globalMatricesUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, globalMatricesUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, NULL, GL_STREAM_DRAW);//sizeof GlobalMatrices struct in .vert
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	//bind a range within a buffer object to an indexed buffer target
	//glBindBufferRange(GLenum 	target,  GLuint 	index_where, 	GLuint buffer_from, GLintptr offset, GLsizeiptr size);
	glBindBufferRange(GL_UNIFORM_BUFFER, globalMatricesBindingIndex, globalMatricesUBO, 0, sizeof(glm::mat4) * 2);

	control->font->UBO = globalMatricesUBO;  //put to constructor or something.. setFunction
	control->font->loadFont("./Fonts/EleganTech-.ttf", 20);*/

	fps = std::unique_ptr<fps_counter>(new fps_counter());
	timestampQuery = std::make_unique<bufferedQuery>(GL_TIMESTAMP);


	glGenTextures(1, &stencilTextureID);
	glBindTexture(GL_TEXTURE_2D, stencilTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, windowWidth, windowHeight, 0, GL_RED_INTEGER, GL_INT, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
		
	GLuint vbo;
	GLuint ibo;
	GLuint shadowVolumeBuffer;
	GLuint shadowVolumeBufferCpu;
	GLuint shadowVolumeComputationInfo;

	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ibo);
	glGenBuffers(1, &shadowVolumeBuffer);
	glGenBuffers(1, &shadowVolumeBufferCpu);
	glGenBuffers(1, &shadowVolumeComputationInfo);
		
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, shadowVolumeBuffer);
	// *8, protože pro každý trojúhelník můžeme potenciálně vygenerovat až 7 dalších trojúhelníků + puvodni
	glBufferData(GL_SHADER_STORAGE_BUFFER, shadowModel.indexCount * sizeof(ShadowVolumeVertex) * 8, nullptr, GL_DYNAMIC_COPY);


	//set up vao for shadow volume display - from gpu
	GLuint shadowVolumeVAO;
	glGenVertexArrays(1, &shadowVolumeVAO);
	glBindVertexArray(shadowVolumeVAO);

		glBindBuffer(GL_ARRAY_BUFFER, shadowVolumeBuffer);

		auto numArrays = 2;

		for (int i = 0; i < numArrays; i++)
			glEnableVertexAttribArray(i);

		// pozice
		glVertexAttribPointer(0u, 4, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(ShadowVolumeVertex), reinterpret_cast<void*>(offsetof(ShadowVolumeVertex, x)));
		// multiplicita
		glVertexAttribIPointer(1u, 1, GL_INT, (GLsizei) sizeof(ShadowVolumeVertex), reinterpret_cast<void*>(offsetof(ShadowVolumeVertex, multiplicity)));


		glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	//set up vao for shadow volume display - from cpu
	GLuint shadowVolumeVAO_CPU;
	glGenVertexArrays(1, &shadowVolumeVAO_CPU);
	glBindVertexArray(shadowVolumeVAO_CPU);

		glBindBuffer(GL_ARRAY_BUFFER, shadowVolumeBufferCpu);

			numArrays = 2;

			for (int i = 0; i < numArrays; i++)
				glEnableVertexAttribArray(i);

			// pozice
			glVertexAttribPointer(0u, 4, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(ShadowVolumeVertex), reinterpret_cast<void*>(offsetof(ShadowVolumeVertex, x)));
			// multiplicita
			glVertexAttribIPointer(1u, 1, GL_INT, (GLsizei) sizeof(ShadowVolumeVertex), reinterpret_cast<void*>(offsetof(ShadowVolumeVertex, multiplicity)));


		glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);


	//set up vao for scene 
	GLuint sceneVAO;
	glGenVertexArrays(1, &sceneVAO);
	glBindVertexArray(sceneVAO);
	
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

			numArrays = 2;

			for (int i = 0; i < numArrays; i++)
				glEnableVertexAttribArray(i);

			// pozice
			glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _x)));
			// normala
			glVertexAttribPointer(1u, 3, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _nx)));



		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(0);


	//set up vao for simple scene to build depth buffer
	GLuint depthVAO;
	glGenVertexArrays(1, &depthVAO);
	glBindVertexArray(depthVAO);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

		numArrays = 1;

		for (int i = 0; i < numArrays; i++)	
			glEnableVertexAttribArray(i);

		// pozice
		glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _x)));



		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(0);



	//set up vao for cpu and gpu stencil computatioon
	GLuint stencilVAO_CPU, stencilVAO_GPU;

	glGenVertexArrays(1, &stencilVAO_CPU);
	glGenVertexArrays(1, &stencilVAO_GPU);

	glBindVertexArray(stencilVAO_CPU);

		glBindBuffer(GL_ARRAY_BUFFER, shadowVolumeBufferCpu); //bind output of cpu impl as array buffer

			numArrays = 2;

			for (int i = 0; i < numArrays; i++)
				glEnableVertexAttribArray(i);

			// pozice vcetne w -> 4 floaty
			glVertexAttribPointer(0u, 4, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(ShadowVolumeVertex), reinterpret_cast<void*>(offsetof(ShadowVolumeVertex, x)));
			// multiplicita
			glVertexAttribIPointer(1u, 1, GL_INT, (GLsizei) sizeof(ShadowVolumeVertex), reinterpret_cast<void*>(offsetof(ShadowVolumeVertex, multiplicity)));


		glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	glBindVertexArray(0);


	glBindVertexArray(stencilVAO_GPU);

		glBindBuffer(GL_ARRAY_BUFFER, shadowVolumeBuffer); //bind output of compute shader as array buffer

		numArrays = 2;

		for (int i = 0; i < numArrays; i++)
			glEnableVertexAttribArray(i);

		// pozice vcetne w -> 4 floaty
		glVertexAttribPointer(0u, 4, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(ShadowVolumeVertex), reinterpret_cast<void*>(offsetof(ShadowVolumeVertex, x)));
		// multiplicita
		glVertexAttribIPointer(1u, 1, GL_INT, (GLsizei) sizeof(ShadowVolumeVertex), reinterpret_cast<void*>(offsetof(ShadowVolumeVertex, multiplicity)));

		glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);


	std::cout << "Generujeme pomocné buffery pro výpočet shadow volume" << std::endl;
	
	GLuint simpleVbo;
	GLuint simpleIbo;
	
	GLuint edgeLookupBuffer;


	glGenBuffers(1, &simpleVbo);
	glBindBuffer(GL_ARRAY_BUFFER, simpleVbo);
	glBufferData(GL_ARRAY_BUFFER, simplifiedVertices.size() * sizeof(SimpleVertex), simplifiedVertices.data(), GL_STATIC_DRAW);
	
	glGenBuffers(1, &simpleIbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, simpleIbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, simplifiedIndices.size() * sizeof(GLuint), simplifiedIndices.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &edgeLookupBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, edgeLookupBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, edgeLookup.size() * sizeof(EdgeLookupNode), edgeLookup.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);



	std::cout << "Vstupujeme do hlavní smyčky" << std::endl;
	
	float modelRoty = 0.0f;
	float roty = 0.0f;
	float rotx = 1.0f;
	float dist = -4.0f;
	
	bool rotate = true;
	bool loadShaders = true;
	
	bool drawShadowVolume = false;
	bool CPU = false;
	
	auto ticks = SDL_GetTicks();
	auto ticksDelta = 0;
	
	unsigned frameCounter = 0;
	unsigned lastDisplayedFrameCounter = 0;
	unsigned frameTickCounter = 0;
	unsigned frameComputationTickCounter = 0;
	
	
	ShadowVolumeComputationInfo shadowVolumeInfo;



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
							glViewport(0, 0, windowWidth, windowHeight);
							glDeleteTextures(1, &stencilTextureID);
							glGenTextures(1, &stencilTextureID);
							glBindTexture(GL_TEXTURE_2D, stencilTextureID);
							glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, windowWidth, windowHeight, 0, GL_RED_INTEGER, GL_INT, nullptr);
							glBindTexture(GL_TEXTURE_2D, 0);
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
						case SDLK_t: drawShadowVolume = !drawShadowVolume; break;
						case SDLK_c: 
							CPU = !CPU; 
							if (CPU) std::cout << "Nyní se používá CPU" << std::endl;
							else std::cout << "Nyní se používá GPU" << std::endl;
							break;
						case SDLK_F5: loadShaders = true; break;
					}
					break;
				
				default: 
					break;
			 }
		}
		
		// Cas a pocitadla snimku a podobne veci //
		{
			auto lastTicks = ticks;
			ticks = SDL_GetTicks();
			ticksDelta = ticks - lastTicks;
			modelRoty += rotate ? 0.0002 * (ticksDelta) : 0.0;
			
			frameCounter++;
			frameTickCounter += ticksDelta;
			
			// jednou za deset vterin
			if (frameTickCounter > 10000)
			{
				auto totalFrames = frameCounter - lastDisplayedFrameCounter;
				auto totalSeconds = frameTickCounter * 0.001;
				auto totalComputationSeconds = frameComputationTickCounter * 0.001;
				
				std::cout << "Vykresleno " << totalFrames << " snímků za " << totalSeconds << " s (tj. " << totalFrames / totalSeconds << " FPS)" << std::endl;
				std::cout << "Průměrná doba vykreslování jendoho snímku: " << totalSeconds / totalFrames << " s" << std::endl;
				std::cout << "Průměrná doba výpočtu stínového tělesa: " << totalComputationSeconds / totalFrames << " s" << std::endl;
				
				lastDisplayedFrameCounter = frameCounter;
				frameTickCounter = 0;
				frameComputationTickCounter = 0;
			}
		}
		
		try
		{

			auto projection = glm::perspective(90.0f, float(windowWidth) / float(windowHeight), 0.1f, 100.0f);
			
			auto translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, dist));
			auto rotate =	glm::rotate(translate, rotx, glm::vec3(1.0f, 0.0f, 0.0f));
			auto view =		glm::rotate(rotate, roty, glm::vec3(0.0f, 1.0f, 0.0f));

			shadowModel.transform =	glm::rotate(glm::mat4(1.0f), modelRoty, glm::vec3(0.0f, 1.0f, 0.0f));

			environmentModel.transform = glm::rotate(glm::mat4(1.0f), modelRoty * 0.25f, glm::vec3(0.0f, 1.0f, 0.0f));

			auto pMat = projection;

			

			{
				unsigned computationStartTicks = SDL_GetTicks();
				auto lightDir = glm::mat3(glm::inverse(shadowModel.transform)) * glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
				shadowVolumeInfo.triCount = 0;

				if (CPU)
				{
					std::vector<ShadowVolumeVertex> shadowVolumeVertices;
					shadowVolumeVertices.reserve(simplifiedModel.indexCount * 7);

					shadowVolumeInfo = compute(
						simplifiedModel.baseIndex,
						simplifiedModel.indexCount,
						lightDir,
						100.0f,
						simplifiedVertices,
						simplifiedIndices,
						edgeLookup,
						shadowVolumeVertices
						);

					glBindBuffer(GL_ARRAY_BUFFER, shadowVolumeBufferCpu);
					glBufferData(GL_ARRAY_BUFFER, shadowVolumeVertices.size() * sizeof(ShadowVolumeVertex), shadowVolumeVertices.data(), GL_STREAM_DRAW);
					//glBufferSubData(GL_ARRAY_BUFFER, 0, shadowVolumeVertices.size() * sizeof(ShadowVolumeVertex), shadowVolumeVertices.data());
					glBindBuffer(GL_ARRAY_BUFFER, 0);

					
				
				}
				else if (volumeComputationProgram.id != 0)
				{
					volumeComputationProgram.useProgram();

					glBindBuffer(GL_SHADER_STORAGE_BUFFER, shadowVolumeComputationInfo);
					glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ShadowVolumeComputationInfo), &shadowVolumeInfo, GL_STREAM_READ);  //nulovani
					//glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ShadowVolumeComputationInfo), &shadowVolumeInfo);
					glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

					auto lightDirLocation = glGetUniformLocation(volumeComputationProgram.id, "lightDir");
					glUniform3fv(lightDirLocation, 1, glm::value_ptr(lightDir));

					glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, simpleVbo);
					glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, simpleIbo);
					glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, shadowVolumeBuffer);
					glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, shadowVolumeComputationInfo);
					glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, edgeLookupBuffer);

					auto indexOffsetLocation = glGetUniformLocation(volumeComputationProgram.id, "indexOffset");
					glUniform1ui(indexOffsetLocation, simplifiedModel.baseIndex);

					auto indexCountLocation = glGetUniformLocation(volumeComputationProgram.id, "indexCount");
					glUniform1ui(indexCountLocation, simplifiedModel.indexCount);

					
						glDispatchCompute((shadowModel.indexCount / 3 + 127) / 128, 1, 1);
						glMemoryBarrier(GL_ALL_BARRIER_BITS);
						
					

					for (unsigned i = 0; i < 5; i++)
						glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, 0);

					glBindBuffer(GL_SHADER_STORAGE_BUFFER, shadowVolumeComputationInfo);		//hadam ze pro ctverec by to melo byt 12 a ne 6
					glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ShadowVolumeComputationInfo), &shadowVolumeInfo);	//precteni nove hodnoty
					glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

					glUseProgram(0);

					
				}

				frameComputationTickCounter += SDL_GetTicks() - computationStartTicks;
			}





			//this bit should clear stencil texture between runs
			GLint clearColor[] = { 0 };
			glBindTexture(GL_TEXTURE_2D, stencilTextureID);
			glClearTexImage(stencilTextureID, 0, GL_RED_INTEGER, GL_INT, nullptr);	//
			glBindTexture(GL_TEXTURE_2D, 0);

			///////////////////////////////////////////////////////////////////////////////////////////////////////

			simpleProgram.useProgram();

			glBindVertexArray(depthVAO);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);	//

		
			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
			glDepthFunc(GL_LESS);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

			auto mvLocation = glGetUniformLocation(simpleProgram.id, "mvMat");
			auto pLocation = glGetUniformLocation(simpleProgram.id, "pMat");



			for (auto modelInfo : scene) {
				auto mvMat = view * modelInfo->transform;
				auto mvNormMat = glm::transpose(glm::inverse(glm::mat3(mvMat)));
				glUniformMatrix4fv(mvLocation, 1, GL_FALSE, glm::value_ptr(mvMat));
				glUniformMatrix4fv(pLocation, 1, GL_FALSE, glm::value_ptr(pMat));
				glDrawElements(GL_TRIANGLES, (GLsizei)modelInfo->indexCount, GL_UNSIGNED_INT, reinterpret_cast<void*>(modelInfo->baseIndex * sizeof(GLuint)));
			}

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindVertexArray(0);

			glUseProgram(0);
			///////////////////////////////////////////////////////////////////////////////////////////////////////

			stencilProgram.useProgram();

			if (CPU){
				glBindVertexArray(stencilVAO_CPU);

			} else {
				glBindVertexArray(stencilVAO_GPU);
				
			}

			

			glDepthMask(GL_FALSE);
			glDepthFunc(GL_LESS);
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);  //just in case
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

			
				glBindImageTexture(0, stencilTextureID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);

				mvLocation = glGetUniformLocation(stencilProgram.id, "mvMat");
				pLocation = glGetUniformLocation(stencilProgram.id, "pMat");

				glUniformMatrix4fv(mvLocation, 1, GL_FALSE, glm::value_ptr(view * shadowModel.transform));
				glUniformMatrix4fv(pLocation, 1, GL_FALSE, glm::value_ptr(pMat));

			
					glDrawArrays(GL_TRIANGLES, 0, shadowVolumeInfo.triCount * 3);


				glBindVertexArray(0);

			glUseProgram(0);

			///////////////////////////////////////////////////////////////////////////////////////////////////////
			
			lightingProgram.useProgram();

				glBindVertexArray(sceneVAO);

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

				
					glDepthMask(GL_TRUE);
					glDepthFunc(GL_LESS);
					glClear(GL_DEPTH_BUFFER_BIT); 
					glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

					//draw scene with lighting

					//reuse previously declared variables
					mvLocation = glGetUniformLocation(lightingProgram.id, "mvMat");
					auto mvNormLocation = glGetUniformLocation(lightingProgram.id, "mvNormMat");
					pLocation = glGetUniformLocation(lightingProgram.id, "pMat");

					glBindImageTexture(0, stencilTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32I);

					glUniformMatrix4fv(pLocation, 1, GL_FALSE, glm::value_ptr(pMat));

					auto lightDirLocation = glGetUniformLocation(lightingProgram.id, "lightDir");

					if (lightDirLocation != -1)
					{
						auto lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
						lightDir = glm::mat3(view) * lightDir;
						glUniform3fv(lightDirLocation, 1, glm::value_ptr(lightDir));
					}


					for (auto modelInfo : scene) {
						auto mvMat = view * modelInfo->transform;
						auto mvNormMat = glm::transpose(glm::inverse(glm::mat3(mvMat)));
						glUniformMatrix4fv(mvLocation, 1, GL_FALSE, glm::value_ptr(mvMat));
						glUniformMatrix3fv(mvNormLocation, 1, GL_FALSE, glm::value_ptr(mvNormMat));
						glDrawElements(GL_TRIANGLES, (GLsizei)modelInfo->indexCount, GL_UNSIGNED_INT, reinterpret_cast<void*>(modelInfo->baseIndex * sizeof(GLuint)));
					}

					
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

				glBindVertexArray(0);

			glUseProgram(0);
	
			if (volumeVisualizationProgram.id != 0 && drawShadowVolume)
			{
				
				volumeVisualizationProgram.useProgram();

					if (CPU){
						glBindVertexArray(shadowVolumeVAO_CPU);

						//glBindBuffer(GL_ARRAY_BUFFER, shadowVolumeBufferCpu); //bind output of cpu impl as array buffer
					}
					else{
						glBindVertexArray(shadowVolumeVAO);

						//glBindBuffer(GL_ARRAY_BUFFER, shadowVolumeBuffer); //bind output of compute shader as array buffer

					}

				


						glEnable(GL_BLEND);
						glBlendFunc(GL_SRC_ALPHA, GL_ONE);
						glDepthMask(GL_FALSE);
			
						auto mvLocation = glGetUniformLocation(volumeVisualizationProgram.id, "mvMat");
						auto mvNormLocation = glGetUniformLocation(volumeVisualizationProgram.id, "mvNormMat");
						auto pLocation = glGetUniformLocation(volumeVisualizationProgram.id, "pMat");
				
						auto mvMat = view * shadowModel.transform;
						auto mvNormMat = glm::transpose(glm::inverse(glm::mat3(mvMat)));
				
						glUniformMatrix4fv(mvLocation, 1, GL_FALSE, glm::value_ptr(mvMat));
						glUniformMatrix3fv(mvNormLocation, 1, GL_FALSE, glm::value_ptr(mvNormMat));
						glUniformMatrix4fv(pLocation, 1, GL_FALSE, glm::value_ptr(pMat));
				

							glDrawArrays(GL_TRIANGLES, 0, shadowVolumeInfo.triCount * 3);
							

					glBindVertexArray(0);
				
				glUseProgram(0);
				
				glDepthMask(GL_TRUE);
			}
		
			SDL_GL_SwapWindow(window);
		}
		catch (std::exception& ex)
		{
			std::cerr << ex.what() << std::endl;
		}
	}
	
	glDeleteTextures(1, &stencilTextureID);

	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ibo);
	glDeleteBuffers(1, &shadowVolumeBuffer);
	glDeleteBuffers(1, &shadowVolumeComputationInfo);

	glDeleteVertexArrays(1, &shadowVolumeVAO);
	glDeleteVertexArrays(1, &depthVAO);
	glDeleteVertexArrays(1, &sceneVAO);
	glDeleteVertexArrays(1, &stencilVAO_CPU);
	glDeleteVertexArrays(1, &stencilVAO_GPU);

	SDL_GL_DeleteContext(glCtx);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
