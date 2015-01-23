#ifndef VERTEXTYPES_H
#define VERTEXTYPES_H

#include <glm/glm.hpp>

struct Vertex
{
	Vertex(
		float x, float y, float z, 
		float nx, float ny, float nz, 
		float u, float v);

	Vertex(
		glm::vec3 point,
		glm::vec3 normal,
		glm::vec2 tex);

	float _x;
	float _y;
	float _z;
	
	float _nx;
	float _ny;
	float _nz;
	
	float _u;
	float _v;
};

static_assert(sizeof(Vertex) == sizeof(float) * 8, "Velikost struktury Vertex neni 8 floatu");

struct SimpleVertex
{
	SimpleVertex(
		float x, float y, float z);

	SimpleVertex(
		glm::vec3 point);
	
	float _x;
	float _y;
	float _z;
    //int padding;
};

struct ShadowVolumeVertex
{
	ShadowVolumeVertex(float x, float y, float z, int multiplicity, unsigned isCap);
	ShadowVolumeVertex(const glm::vec3& point, int multiplicity, unsigned isCap);
	
	float x;
	float y;
	float z;
	float w;
	int multiplicity;
	int isCap;
	//int padding[2];
};

//static_assert(sizeof(ShadowVolumeVertex) == sizeof(float) * 8, "Velikost struktury ShadowVolumeVertex neni 8 floatu");

#endif
