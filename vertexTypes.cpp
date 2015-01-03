#include "vertexTypes.h"

Vertex::Vertex(
	float x, float y, float z, 
	float nx, float ny, float nz,
	float u, float v):
	_x(x),
	_y(y),
	_z(z),
	
	_nx(nx),
	_ny(ny),
	_nz(nz),
	
	_u(u),
	_v(v)
{}

Vertex::Vertex(
	glm::vec3 point,
	glm::vec3 normal,
	glm::vec2 tex){


	_x = point.x;
	_y = point.y;
	_z = point.z;

	_nx = normal.x;
	_ny = normal.y;
	_nz = normal.z;

	_u = tex.x;
	_v = tex.y;

}

SimpleVertex::SimpleVertex(float x, float y, float z):
	_x(x),
	_y(y),
	_z(z)
{}

SimpleVertex::SimpleVertex(glm::vec3 position):
	_x(position.x),
	_y(position.y),
	_z(position.z)
{}

ShadowVolumeVertex::ShadowVolumeVertex(float x, float y, float z, int multiplicity, unsigned isCap):
	x(x),
	y(y),
	z(z),
	multiplicity(multiplicity),
	isCap(isCap)
{}

ShadowVolumeVertex::ShadowVolumeVertex(const glm::vec3& point, int multiplicity, unsigned isCap):
	x(point.x),
	y(point.y),
	z(point.z),
	multiplicity(multiplicity),
	isCap(isCap)
{}
