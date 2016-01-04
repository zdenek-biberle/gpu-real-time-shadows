
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

// volume computation method
enum class Method
{
	cpu, gpuCompute, gpuGeometry
};

//1     2      3     4      5     6      7		  8
//scene caster blend method resolution   duration distance
//*.obj *.obj  0/1   0-2    width height seconds  units

int wrapped_main(int argc, char** argv)
{
	if (argc < 3)
	{
		throw std::runtime_error("Nesprávný počet parametrů. Správné použité je: \npgpu-real-time-shadows <model scény> <stínící model>");
	}

	auto environmentModelFilename = std::string(argv[1]);
	auto shadowModelFilename = std::string(argv[2]);
	vec3 lightDirection = glm::normalize(vec3(-1, -1, -0.25)); //smerova svetla pro jednoduchost
	SDL_Init(SDL_INIT_VIDEO);

	float duration = -1;
	float dist = -4.0f;
	Method method = Method::cpu;
	bool blend = false;
	int windowWidth = 1024;
	int windowHeight = 768;

	if (argc == 9) {
		duration = atof(argv[7]);
		dist = -atof(argv[8]);
		
		blend = atoi(argv[3]);
		windowWidth = atoi(argv[5]);
		windowHeight = atoi(argv[6]);
	
		switch (atoi(argv[4])) {
			case 0: method = Method::cpu; break;
			case 1: method = Method::gpuCompute; break;
			case 2: method = Method::gpuGeometry; break;
			default: method = Method::cpu; break;

		}
		
	}

	auto window = SDL_CreateWindow(
		"GPU real time shadows",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		windowWidth, windowHeight,
		SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	if (!window)
	{
		// nepodarilo se vytvorit okno
		std::ostringstream stream;
		stream << "SDL_CreateWindow error: " << SDL_GetError() << std::endl;
		throw std::runtime_error(stream.str());
	}
		
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);	//must be enabled to allow debug output callback
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	
	auto glCtx = SDL_GL_CreateContext(window);
	
	glewExperimental = GL_TRUE;
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
		std::cerr << "Neni k dispozici debug output" << std::endl;
	}
	
	
	std::cout << "Loadujeme modely" << std::endl;
	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;

	auto environmentModel = loadModel(environmentModelFilename, vertices, indices);
	auto shadowModel = loadModel(shadowModelFilename, vertices, indices);

	environmentModel.color = vec3(0.8, 0.2, 0.1);
	shadowModel.color = vec3(0.2, 0.8, 0.1);
	std::cout << "Zjednodusujeme stinici model" << std::endl;
	
	std::vector<SimpleVertex> simplifiedVertices;
	std::vector<GLuint> simplifiedIndices;
	auto simplifiedModel = simplifyModel(shadowModel, vertices, indices, simplifiedVertices, simplifiedIndices);
	
	std::cout << "Generujeme buffer pro vyhledavani hran" << std::endl;
	
	std::vector<EdgeLookupNode> edgeLookup;
	generateEdgeLookup(simplifiedModel, simplifiedIndices, edgeLookup);
	
	std::vector<decltype(environmentModel)*> scene = { &environmentModel, &shadowModel };
	

	std::unique_ptr<fps_counter> fps;
	std::unique_ptr<bufferedQuery> timestampQuery;

	std::cout << "Creating buffers" << std::endl;

	GLuint stencilTextureID;

	//add programs to control..
	std::cout << "Kompilujeme shadery" << std::endl;
	Control *control = Control::getInstance();

	control->recomputeProjections(windowWidth, windowHeight);

	control->addProgram("simple");
	control->addProgram("stencil");
	control->addProgram("lighting");
	control->addProgram("volumeComputation");
	control->addProgram("volumeComputationGeometry");
	control->addProgram("volumeVisualization");
	control->addProgram("volumeVisualizationGeometry");
	control->addProgram("font");
	control->addProgram("caster");

	{
		ShaderProgram *fontProgram = control->getProgram("font");

		fontProgram->addShader(GL_VERTEX_SHADER, "./glsl/font/FontVS.vert");
		fontProgram->addShader(GL_FRAGMENT_SHADER, "./glsl/font/FontFS.frag");

		if (!fontProgram->linkProgram()) {
			std::cin.ignore();
			exit(1);
		}
	}

	{
		ShaderProgram *simpleProgram = control->getProgram("simple");

		simpleProgram->addShader(GL_VERTEX_SHADER, "./glsl/scene/simple.vert");
		simpleProgram->addShader(GL_FRAGMENT_SHADER, "./glsl/scene/simple.frag");

		if (!simpleProgram->linkProgram()){
			std::cin.ignore();
			exit(1);
		}
	}

	{
		ShaderProgram *stencilProgram = control->getProgram("stencil");

		stencilProgram->addShader(GL_VERTEX_SHADER, "./glsl/scene/stencil.vert");
		stencilProgram->addShader(GL_FRAGMENT_SHADER, "./glsl/scene/stencil.frag");

		if (!stencilProgram->linkProgram()){
			std::cin.ignore();
			exit(1);
		}
	}

	{
		ShaderProgram *lightingProgram = control->getProgram("lighting");

		lightingProgram->addShader(GL_VERTEX_SHADER, "./glsl/scene/vert.glsl");
		lightingProgram->addShader(GL_FRAGMENT_SHADER, "./glsl/scene/frag.glsl");

		if (!lightingProgram->linkProgram()){
			std::cin.ignore();
			exit(1);
		}
	}
	
	{
		ShaderProgram *casterProgram = control->getProgram("caster");

		casterProgram->addShader(GL_VERTEX_SHADER, "./glsl/scene/caster.vert");
		casterProgram->addShader(GL_FRAGMENT_SHADER, "./glsl/scene/caster.frag");

		if (!casterProgram->linkProgram()) {
			std::cin.ignore();
			exit(1);
		}
	}

	{
		ShaderProgram *volumeComputationProgram = control->getProgram("volumeComputation");

		volumeComputationProgram->addShader(GL_COMPUTE_SHADER, "./glsl/volume-computation-common/common.glsl");
		volumeComputationProgram->addShader(GL_COMPUTE_SHADER, "./glsl/volume-computation/compute.glsl");

		if (!volumeComputationProgram->linkProgram()) {
			std::cin.ignore();
			exit(1);
		}
	}

	{
		ShaderProgram *volumeVisualizationProgram = control->getProgram("volumeVisualization");

		volumeVisualizationProgram->addShader(GL_VERTEX_SHADER, "./glsl/volume-visualization/vert.glsl");
		volumeVisualizationProgram->addShader(GL_FRAGMENT_SHADER, "./glsl/volume-visualization/frag.glsl");

		if (!volumeVisualizationProgram->linkProgram()) {
			std::cin.ignore();
			exit(1);
		}
	}
	
	{
		auto volumeComputationGeometryProgram = control->getProgram("volumeComputationGeometry");
		
		volumeComputationGeometryProgram->addShader(GL_VERTEX_SHADER, "./glsl/volume-computation-geometry/vert.glsl");
		volumeComputationGeometryProgram->addShader(GL_GEOMETRY_SHADER, "./glsl/volume-computation-common/common.glsl");
		volumeComputationGeometryProgram->addShader(GL_GEOMETRY_SHADER, "./glsl/volume-computation-geometry/geometry.glsl");
		volumeComputationGeometryProgram->addShader(GL_FRAGMENT_SHADER, "./glsl/volume-computation-geometry/frag.glsl");
		
		if (!volumeComputationGeometryProgram->linkProgram()) {
			std::cin.ignore();
			exit(1);
		}
	}

	{
		auto volumeVisualizationGeometryProgram = control->getProgram("volumeVisualizationGeometry");
		
		volumeVisualizationGeometryProgram->addShader(GL_VERTEX_SHADER, "./glsl/volume-computation-geometry/vert.glsl");
		volumeVisualizationGeometryProgram->addShader(GL_GEOMETRY_SHADER, "./glsl/volume-computation-common/common.glsl");
		volumeVisualizationGeometryProgram->addShader(GL_GEOMETRY_SHADER, "./glsl/volume-computation-geometry/geometry.glsl");
		volumeVisualizationGeometryProgram->addShader(GL_FRAGMENT_SHADER, "./glsl/volume-visualization-geometry/frag.glsl");
		
		if (!volumeVisualizationGeometryProgram->linkProgram()) {
			std::cin.ignore();
			exit(1);
		}
	}
	
	//create font sampler
	Sampler *sampler = control->addSampler("font");

	glSamplerParameteri(sampler->id, GL_TEXTURE_MIN_LOD, 0);
	glSamplerParameteri(sampler->id, GL_TEXTURE_MAX_LOD, 0);

	glSamplerParameteri(sampler->id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  //want to have sharp font
	glSamplerParameteri(sampler->id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	//glSamplerParameteri(sampler->id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glSamplerParameteri(sampler->id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glSamplerParameteri(sampler->id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(sampler->id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	
	//essentials should be part of constructor
	control->font = std::make_unique<Font>();
	control->font->addProgram(control->getProgram("font")->id);

	GLuint globalMatricesBindingIndex = 0;

	glUniformBlockBinding(control->getProgram("font")->id, glGetUniformBlockIndex(control->getProgram("font")->id, "GlobalMatrices"), globalMatricesBindingIndex);
	
	GLuint globalMatricesUBO;

	//create uniform buffer
	glGenBuffers(1, &globalMatricesUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, globalMatricesUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, NULL, GL_STREAM_DRAW);//sizeof GlobalMatrices struct in .vert
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	//bind a range within a buffer object to an indexed buffer target
	//glBindBufferRange(GLenum 	target,  GLuint 	index_where, 	GLuint buffer_from, GLintptr offset, GLsizeiptr size);
	glBindBufferRange(GL_UNIFORM_BUFFER, globalMatricesBindingIndex, globalMatricesUBO, 0, sizeof(glm::mat4) * 2);

	control->font->UBO = globalMatricesUBO;  //put to constructor or something.. setFunction
	control->font->sampler = sampler;

	control->font->loadFont("./Fonts/EleganTech-.ttf", 20);


	fps = std::make_unique<fps_counter>(20);
	timestampQuery = std::make_unique<bufferedQuery>(GL_TIMESTAMP);

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &stencilTextureID);
	glBindTexture(GL_TEXTURE_2D, stencilTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, windowWidth, windowHeight, 0, GL_RED_INTEGER, GL_INT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

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
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, shadowVolumeBuffer);
	// *8, protože pro každý trojúhelník můžeme potenciálně vygenerovat až 7 dalších trojúhelníků + puvodni
	glBufferData(GL_SHADER_STORAGE_BUFFER, shadowModel.indexCount * sizeof(ShadowVolumeVertex) * 8, nullptr, GL_DYNAMIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	
	std::cout << "Generujeme pomocne buffery pro vypocet shadow volume" << std::endl;
	
	GLuint simpleVbo;
	GLuint simpleIbo;
	
	GLuint edgeLookupBuffer;

	glGenBuffers(1, &simpleVbo);
	glBindBuffer(GL_ARRAY_BUFFER, simpleVbo);
	glBufferData(GL_ARRAY_BUFFER, simplifiedVertices.size() * sizeof(SimpleVertex), simplifiedVertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	glGenBuffers(1, &simpleIbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, simpleIbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, simplifiedIndices.size() * sizeof(GLuint), simplifiedIndices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	glGenBuffers(1, &edgeLookupBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, edgeLookupBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, edgeLookup.size() * sizeof(EdgeLookupNode), edgeLookup.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	
	
	std::cout << "Generujeme VAO" << std::endl;
	
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
	
	//set up vao for shadow volume display - from gpu, via geometry shaders
	GLuint shadowVolumeVAO_CPU_geometry;
	glGenVertexArrays(1, &shadowVolumeVAO_CPU_geometry);
	glBindVertexArray(shadowVolumeVAO_CPU_geometry);

		glBindBuffer(GL_ARRAY_BUFFER, simpleVbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, simpleIbo);

		glEnableVertexAttribArray(0);

		// pozice
		glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(SimpleVertex), reinterpret_cast<void*>(offsetof(SimpleVertex, _x)));

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

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

	// setup VAO for rendering the shadow volume via geometry shaders
	GLuint stencilVAO_GPU_geometry;
	glGenVertexArrays(1, &stencilVAO_GPU_geometry);
	glBindVertexArray(stencilVAO_GPU_geometry);
	
		glBindBuffer(GL_ARRAY_BUFFER, simpleVbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, simpleIbo);

		glEnableVertexAttribArray(0);

		// pozice
		glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, (GLsizei) sizeof(SimpleVertex), reinterpret_cast<void*>(offsetof(SimpleVertex, _x)));

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	glBindVertexArray(0);
	
	
	std::cout << "Vstupujeme do hlavni smycky" << std::endl;
	
	float modelRoty = 0.0f;
	float roty = 0.0f;
	float rotx = 1.0f;


	bool rotate = true;
	
	bool drawShadowVolume = false;

	
	auto ticks = SDL_GetTicks();
	auto ticksDelta = 0;
	
	unsigned frameCounter = 0;
	unsigned lastDisplayedFrameCounter = 0;
	unsigned frameTickCounter = 0;
	unsigned frameComputationTickCounter = 0;
	time_item start = time_item();

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

							control->recomputeProjections(windowWidth, windowHeight, 70.0);

							glViewport(0, 0, windowWidth, windowHeight);

							glActiveTexture(GL_TEXTURE0);
							glDeleteTextures(1, &stencilTextureID);
							glGenTextures(1, &stencilTextureID);
							glBindTexture(GL_TEXTURE_2D, stencilTextureID);
							glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, windowWidth, windowHeight, 0, GL_RED_INTEGER, GL_INT, nullptr);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
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

							if (method == Method::cpu) {
								method = Method::gpuCompute;
								std::cout << "Nyni se pouziva GPU a compute shadery" << std::endl;
							
							} else if (method == Method::gpuCompute) {
								method = Method::gpuGeometry;
								std::cout << "Nyni se pouziva GPU a geometry shadery" << std::endl;

							} else if (method == Method::gpuGeometry) {
					
								method = Method::cpu;
								std::cout << "Nyni se pouziva CPU" << std::endl;
							}

							
							break;
						
							
						case SDLK_F5: control->recompileAllPrograms(); break;

						case SDLK_i: control->stats = !control->stats; break;

						case SDLK_b: blend = !blend; break; 
						
						case SDLK_ESCAPE: run = false;
					}
					break;
				
				default: 
					break;
			 }
		}
		
		time_item now = time_item();
		if (duration > 0 && duration < now.seconds(start))
			run = false;

		glQueryCounter(timestampQuery->query(), GL_TIMESTAMP);

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
				
				lastDisplayedFrameCounter = frameCounter;
				frameTickCounter = 0;
				frameComputationTickCounter = 0;
			}
			
			lightDirection = glm::normalize(vec3(std::sin(ticks * 0.001), -1, -std::cos(ticks * 0.001)));
		}
		


		try
		{
			auto translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, dist));
			auto rotate =	 glm::rotate(translate, rotx, glm::vec3(1.0f, 0.0f, 0.0f));
			auto view =		 glm::rotate(rotate, roty, glm::vec3(0.0f, 1.0f, 0.0f));

			shadowModel.transform =	glm::rotate(glm::mat4(1.0f), modelRoty, glm::vec3(0.0f, 1.0f, 0.0f));

			environmentModel.transform = glm::rotate(glm::mat4(1.0f), modelRoty * 0.25f, glm::vec3(0.0f, 1.0f, 0.0f));

			auto pMat = control->perspectiveMatrix;

			
			///////////////////////////////////////////////////////////////////////////////////////
			/*
			Compute cast. CPU a GPU by mely delat totez.
			*/

			{
				unsigned computationStartTicks = SDL_GetTicks();

				if (method == Method::cpu)
				{
					std::vector<ShadowVolumeVertex> shadowVolumeVertices;
					shadowVolumeVertices.reserve(simplifiedModel.indexCount * 7);

					shadowVolumeInfo = compute(
						simplifiedModel.baseIndex,
						simplifiedModel.indexCount,
						lightDirection,
						50000.0f,
						simplifiedVertices,
						simplifiedIndices,
						edgeLookup,
						shadowVolumeVertices
						);

					glBindBuffer(GL_ARRAY_BUFFER, shadowVolumeBufferCpu);
					glBufferData(GL_ARRAY_BUFFER, shadowVolumeVertices.size() * sizeof(ShadowVolumeVertex), shadowVolumeVertices.data(), GL_STREAM_READ);   
					//glBufferSubData(GL_ARRAY_BUFFER, 0, shadowVolumeVertices.size() * sizeof(ShadowVolumeVertex), shadowVolumeVertices.data());
					glBindBuffer(GL_ARRAY_BUFFER, 0);

					
				
				}
				else if (method == Method::gpuCompute && control->getProgram("volumeComputation")->id != 0)
				{
					ShaderProgram *program = control->getProgram("volumeComputation");

					program->useProgram();

					shadowVolumeInfo.triCount = 0;

					glBindBuffer(GL_SHADER_STORAGE_BUFFER, shadowVolumeComputationInfo);
					glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ShadowVolumeComputationInfo), &shadowVolumeInfo, GL_STREAM_READ);  //nulovani	
					//glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ShadowVolumeComputationInfo), &shadowVolumeInfo);
					glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

					auto lightDirLocation = glGetUniformLocation(program->id, "lightDir");
					glUniform3fv(lightDirLocation, 1, glm::value_ptr(lightDirection));

					glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, simpleVbo);
					glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, simpleIbo);
					glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, edgeLookupBuffer);
					glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, shadowVolumeBuffer);
					glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, shadowVolumeComputationInfo);

					auto indexOffsetLocation = glGetUniformLocation(program->id, "indexOffset");
					glUniform1ui(indexOffsetLocation, simplifiedModel.baseIndex);

					auto indexCountLocation = glGetUniformLocation(program->id, "indexCount");
					glUniform1ui(indexCountLocation, simplifiedModel.indexCount);

					glDispatchCompute((shadowModel.indexCount / 3 + 127) / 128, 1, 1);
					glMemoryBarrier(GL_ALL_BARRIER_BITS);

					for (unsigned i = 0; i < 5; i++)
						glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, 0);

					glBindBuffer(GL_SHADER_STORAGE_BUFFER, shadowVolumeComputationInfo);	
					glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ShadowVolumeComputationInfo), &shadowVolumeInfo);	//precteni nove hodnoty - spravnych 10 - 2 backcap a 8 side caps
					glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

					glUseProgram(0);
				}

				frameComputationTickCounter += SDL_GetTicks() - computationStartTicks;
			}


			///////////////////////////////////////////////////////////////////////////////////////
			/*
			Vykreslovaci cast. Uvedeni do vychoziho stavu.
			*/

			glDisable(GL_CULL_FACE);
			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
			glDepthFunc(GL_LESS);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			//this bit should clear stencil texture between runs
			//GLint clearColor[] = { 0 };
			//glBindTexture(GL_TEXTURE_2D, stencilTextureID);
			//GLuint texture,	GLint level,	GLenum format,	GLenum type,	const void * data
			glClearTexImage(stencilTextureID, 0, GL_RED_INTEGER, GL_INT, nullptr);	//
			//glBindTexture(GL_TEXTURE_2D, 0);

			///////////////////////////////////////////////////////////////////////////////////////////////////////
			/*
			Vykresleni pouze s prazdnymi shadery pro naplneni depth bufferu. - Pouze prostredi bez shadow casteru
			*/

			ShaderProgram *program = control->getProgram("simple");

			program->useProgram();
			
			glBindVertexArray(depthVAO);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);	//
			
			//nepotrebujeme zaznamenavat barvu
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

			auto mvLocation = glGetUniformLocation(program->id, "mvMat");
			auto pLocation = glGetUniformLocation(program->id, "pMat");
			
			
			//for (auto model : scene)
			{
				auto& model = scene[0];
				auto mvMat = view * model->transform;
				auto mvNormMat = glm::transpose(glm::inverse(glm::mat3(mvMat)));
				glUniformMatrix4fv(mvLocation, 1, GL_FALSE, glm::value_ptr(mvMat));
				glUniformMatrix4fv(pLocation, 1, GL_FALSE, glm::value_ptr(pMat));
				
				glDrawElements(GL_TRIANGLES, (GLsizei) model->indexCount, GL_UNSIGNED_INT, reinterpret_cast<void*>(model->baseIndex * sizeof(GLuint)));
			}
			
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindVertexArray(0);

			glUseProgram(0);

			///////////////////////////////////////////////////////////////////////////////////////////////////////
			/*
			 * Vykresleni vypocitaneho shadow volume v compute casti a ulozeni multiplicity pro kazdu zasazeny pixel.
			 * Zavisle na metode.
			 */
			switch (method)
			{
				case Method::cpu:
				case Method::gpuCompute:
				{
					program = control->getProgram("stencil");

					program->useProgram();

					if (method == Method::cpu){
						glBindVertexArray(stencilVAO_CPU);

					} else {
						glBindVertexArray(stencilVAO_GPU);
						
					}
					
					glEnable(GL_DEPTH_TEST);
					glDepthMask(GL_FALSE);
					glDepthFunc(GL_GEQUAL);
					
					glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);  

						glBindImageTexture(0, stencilTextureID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);

						mvLocation = glGetUniformLocation(program->id, "mvMat");
						pLocation = glGetUniformLocation(program->id, "pMat");

						glUniformMatrix4fv(mvLocation, 1, GL_FALSE, glm::value_ptr(view * shadowModel.transform));
						glUniformMatrix4fv(pLocation, 1, GL_FALSE, glm::value_ptr(pMat));

					
							glDrawArrays(GL_TRIANGLES, 0, shadowVolumeInfo.triCount * 3);


						glBindVertexArray(0);

					glUseProgram(0);
					break;
				}
				
				case Method::gpuGeometry:
				{
					program = control->getProgram("volumeComputationGeometry");
					program->useProgram();
					glBindVertexArray(stencilVAO_GPU_geometry);
					
					//nepotrebujeme zaznamenavat barvu
					glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
					glEnable(GL_DEPTH_TEST);
					glDepthMask(GL_FALSE);
					glDepthFunc(GL_GEQUAL);

					glBindImageTexture(0, stencilTextureID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
					glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, simpleVbo);
					glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, simpleIbo);
					glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, edgeLookupBuffer);
					
					auto mvLocation = glGetUniformLocation(program->id, "mvMat");
					auto pLocation = glGetUniformLocation(program->id, "pMat");

					auto mvMat = view * scene[1]->transform;
					auto mvNormMat = glm::transpose(glm::inverse(glm::mat3(mvMat)));
					glUniformMatrix4fv(mvLocation, 1, GL_FALSE, glm::value_ptr(mvMat));
					glUniformMatrix4fv(pLocation, 1, GL_FALSE, glm::value_ptr(pMat));
					
					auto lightDirLocation = glGetUniformLocation(program->id, "lightDir");
					glUniform3fv(lightDirLocation, 1, glm::value_ptr(lightDirection));
					
					auto indexCountLocation = glGetUniformLocation(program->id, "indexCount");
					glUniform1ui(indexCountLocation, simplifiedModel.indexCount);
					
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, simpleIbo);
					glDrawElements(GL_TRIANGLES, (GLsizei) simplifiedModel.indexCount, GL_UNSIGNED_INT, reinterpret_cast<void*>(simplifiedModel.baseIndex * sizeof(GLuint)));
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
					
					glBindVertexArray(0);
					glUseProgram(0);
					break;
				}
			}

			///////////////////////////////////////////////////////////////////////////////////////////////////////
			/*
			 *Vykresleni sceny se vsim vsudy. Vyuziva pro vykresleni stinu informace ze "stencil" textury.
			 */
			glClear(GL_DEPTH_BUFFER_BIT); 
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
			glDepthFunc(GL_LESS);
			glClear(GL_DEPTH_BUFFER_BIT);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			{
				program = control->getProgram("lighting");

				program->useProgram();
					glBindVertexArray(sceneVAO);

						glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
						
						if (blend)
						glUniform1f(glGetUniformLocation(program->id, "shadowCasterOpacity"), 0.25);
						else
						glUniform1f(glGetUniformLocation(program->id, "shadowCasterOpacity"), 1.0);

						//draw scene with lighting
						//reuse previously declared variables
						mvLocation = glGetUniformLocation(program->id, "mvMat");
						auto mvNormLocation = glGetUniformLocation(program->id, "mvNormMat");
						pLocation = glGetUniformLocation(program->id, "pMat");

						//	GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format
						glBindImageTexture(0, stencilTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32I);

						glUniformMatrix4fv(pLocation, 1, GL_FALSE, glm::value_ptr(pMat));
						
						auto lightDirLocation = glGetUniformLocation(program->id, "lightDir");
						auto colorLocation = glGetUniformLocation(program->id, "color");
						
						auto mvMat = view * scene[0]->transform;
						auto mvNormMat = glm::transpose(glm::inverse(glm::mat3(mvMat)));
						auto transformedLightDirection = glm::mat3(view * scene[1]->transform) * lightDirection;
						
						glUniformMatrix4fv(mvLocation, 1, GL_FALSE, glm::value_ptr(mvMat));
						glUniformMatrix3fv(mvNormLocation, 1, GL_FALSE, glm::value_ptr(mvNormMat));
						glUniform3fv(lightDirLocation, 1, glm::value_ptr(transformedLightDirection));
						glUniform3fv(colorLocation, 1, glm::value_ptr(scene[0]->color));

						glDrawElements(GL_TRIANGLES, (GLsizei) scene[0]->indexCount, GL_UNSIGNED_INT, reinterpret_cast<void*>(scene[0]->baseIndex * sizeof(GLuint)));

						glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

					glBindVertexArray(0);
				glUseProgram(0);
			}

			{
				program = control->getProgram("caster");

				program->useProgram();
					glBindVertexArray(sceneVAO);

						glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
						
						//draw scene with lighting
						//reuse previously declared variables
						mvLocation = glGetUniformLocation(program->id, "mvMat");
						auto mvNormLocation = glGetUniformLocation(program->id, "mvNormMat");
						pLocation = glGetUniformLocation(program->id, "pMat");

						//	GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format
						glBindImageTexture(0, stencilTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32I);

						glUniformMatrix4fv(pLocation, 1, GL_FALSE, glm::value_ptr(pMat));

						auto lightDirLocation = glGetUniformLocation(program->id, "lightDir");
						auto colorLocation = glGetUniformLocation(program->id, "color");
						
						auto mvMat = view * scene[1]->transform;
						auto mvNormMat = glm::transpose(glm::inverse(glm::mat3(mvMat)));
						auto transformedLightDirection = glm::mat3(mvMat) * lightDirection;
						
						glUniformMatrix4fv(mvLocation, 1, GL_FALSE, glm::value_ptr(mvMat));
						glUniformMatrix3fv(mvNormLocation, 1, GL_FALSE, glm::value_ptr(mvNormMat));
						glUniform3fv(lightDirLocation, 1, glm::value_ptr(transformedLightDirection));
						glUniform3fv(colorLocation, 1, glm::value_ptr(scene[1]->color));

						if (blend)
						{
							glDisable(GL_DEPTH_TEST);
							glEnable(GL_BLEND);
							glBlendFunc(GL_SRC_ALPHA, GL_ONE);
						}
						

						
						glDrawElements(GL_TRIANGLES, (GLsizei) scene[1]->indexCount, GL_UNSIGNED_INT, reinterpret_cast<void*>(scene[1]->baseIndex * sizeof(GLuint)));
						
						if (blend)
						{
							glDisable(GL_BLEND);
						}
						
						glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			
					glBindVertexArray(0);
				glUseProgram(0);
			}
			
			glDisable(GL_CULL_FACE);
			
			///////////////////////////////////////////////////////////////////////////////////////////////////////
			/*
			 * Vizualizace shadow volume.
			 */

			if (control->getProgram("volumeVisualization")->id != 0 && drawShadowVolume)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				glDepthMask(GL_FALSE);
			
				switch(method)
				{
					case Method::cpu:
					case Method::gpuCompute:
					{
						ShaderProgram *program = control->getProgram("volumeVisualization");

						program->useProgram();

						if (method == Method::cpu){
							glBindVertexArray(shadowVolumeVAO_CPU);
						}
						else{
							glBindVertexArray(shadowVolumeVAO);
						}
						
						auto mvLocation = glGetUniformLocation(program->id, "mvMat");
						auto mvNormLocation = glGetUniformLocation(program->id, "mvNormMat");
						auto pLocation = glGetUniformLocation(program->id, "pMat");
						
						auto mvMat = view * shadowModel.transform;
						auto mvNormMat = glm::transpose(glm::inverse(glm::mat3(mvMat)));
						
						glUniformMatrix4fv(mvLocation, 1, GL_FALSE, glm::value_ptr(mvMat));
						glUniformMatrix3fv(mvNormLocation, 1, GL_FALSE, glm::value_ptr(mvNormMat));
						glUniformMatrix4fv(pLocation, 1, GL_FALSE, glm::value_ptr(pMat));
						
						glDrawArrays(GL_TRIANGLES, 0, shadowVolumeInfo.triCount * 3);
						
						break;
					}
					
					case Method::gpuGeometry:
					{
						ShaderProgram *program = control->getProgram("volumeVisualizationGeometry");
						program->useProgram();
						glBindVertexArray(shadowVolumeVAO_CPU_geometry);
						
						glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, simpleVbo);
						glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, simpleIbo);
						glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, edgeLookupBuffer);
						
						auto mvLocation = glGetUniformLocation(program->id, "mvMat");
						auto pLocation = glGetUniformLocation(program->id, "pMat");

						auto mvMat = view * scene[1]->transform;
						auto mvNormMat = glm::transpose(glm::inverse(glm::mat3(mvMat)));
						glUniformMatrix4fv(mvLocation, 1, GL_FALSE, glm::value_ptr(mvMat));
						glUniformMatrix4fv(pLocation, 1, GL_FALSE, glm::value_ptr(pMat));
						
						auto lightDirLocation = glGetUniformLocation(program->id, "lightDir");
						glUniform3fv(lightDirLocation, 1, glm::value_ptr(lightDirection));
						
						auto indexCountLocation = glGetUniformLocation(program->id, "indexCount");
						glUniform1ui(indexCountLocation, simplifiedModel.indexCount);
						
						glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, simpleIbo);
						glDrawElements(GL_TRIANGLES, (GLsizei) simplifiedModel.indexCount, GL_UNSIGNED_INT, reinterpret_cast<void*>(simplifiedModel.baseIndex * sizeof(GLuint)));
						glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
						
						break;
					}
				}
				
				glBindVertexArray(0);
				glUseProgram(0);
				glDepthMask(GL_TRUE);
				glDisable(GL_BLEND);
			}
		
			
			///////////////////////////////////////////////////////////////////////////////////////////////////////
			/*
			 * Na zaver vykresleni statistik behu.
			 */
			control->getProgram("font")->useProgram();

			glm::mat4 camMatrix(1.0f);


			glBindBuffer(GL_UNIFORM_BUFFER, globalMatricesUBO);

			glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(camMatrix));
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(control->orthographicMatrix));

			glBindBuffer(GL_UNIFORM_BUFFER, 0);



			if (control->stats)
				fps->stats->printAndSwapBuffers();


			//get result of previous timestamp query
			timestampQuery->getResult(fps->current_time);

			//update frame count and fps values
			//+ dynamic info text if enabled in control.
			fps->update();

			control->frame_number = fps->getFrameCount();



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
	glDeleteBuffers(1, &globalMatricesUBO);

	glDeleteVertexArrays(1, &shadowVolumeVAO);
	glDeleteVertexArrays(1, &depthVAO);
	glDeleteVertexArrays(1, &sceneVAO);
	glDeleteVertexArrays(1, &stencilVAO_CPU);
	glDeleteVertexArrays(1, &stencilVAO_GPU);

	SDL_GL_DeleteContext(glCtx);
	SDL_DestroyWindow(window);
	SDL_Quit();

	std::cout << "Prumerny cas snimku: " << fps->getAvgRenderTime() << " ms => " << fps->getAvgFps() << " fps\nStisknete Enter pro ukonceni." << std::endl;
	std::cin.ignore();
	return 0;
}

int main(int argc, char** argv)
{
	try
	{
		return wrapped_main(argc, argv);
	}
	catch (std::exception& e)
	{
		std::cerr << "Unhandled exception: \n" << e.what() << std::endl;
		throw;
	}
	catch (...)
	{
		std::cerr << "Unhandled unknown exception type" << std::endl;
		throw;
	}
}
