#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3 * 8) out; // 3 * 8 -> maximalne osm trianglu

struct InVertex
{
	float x;
	float y;
	float z;
	float padding;
};

struct EdgeLookupNode
{
	uint idx0;
	uint idx1;
	uint idx2;
	uint triangleIdx;
};

layout (std430, binding = 0) readonly buffer InVertices
{
	InVertex inVertices[];
};

layout (std430, binding = 1) readonly buffer InIndices
{
	uint inIndices[];
};

layout (std430, binding = 2) readonly buffer EdgeLookupBuffer
{
	EdgeLookupNode edgeLookup[];
};

uniform mat4 pMat;
uniform mat4 mvMat;

void main()
{
	// TODO
}