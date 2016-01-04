#version 450

layout (local_size_x = 128) in;

uniform uint indexCount;
uniform uint indexOffset;
uniform float extrusionDistance = 50000.0;
uniform vec3 lightDir;

struct InVertex
{
	float x;
	float y;
	float z;
	float padding;
};

struct OutVertex
{
	vec4 position;
	int multiplicity;
	int[3] padding;

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

layout (std430, binding = 3) writeonly buffer OutVertices
{
	OutVertex outVertices[];
};

layout (std430, binding = 4) writeonly buffer OutInfo
{
	uint outTriCount;
};

void generateShadowVolume(
	uint triangleIdx,
	uint indexCount,
	float extrusionDistance,
	vec3 lightDir, 
	InVertex vertices[3], 
	uint indices[3]);

// Rezervuje trojuhelniky pro emitTriangle. Vhodne pro n > 1.
uint reserveTriangles(uint n)
{
	return atomicAdd(outTriCount, n);
}

void emitTriangle(uint idx, vec3 a, vec3 b, vec3 c, int multiplicity)
{
	idx *= 3;  //prevedeni indexu trojuhelniku na index vrcholu
	
	OutVertex outVertex1;
	OutVertex outVertex2;
	OutVertex outVertex3;

	outVertex1.multiplicity = multiplicity;
	outVertex1.position = vec4(a, 1.0);
	
	outVertex2.multiplicity = multiplicity;
	outVertex2.position = vec4(b, 1.0);
	
	outVertex3.multiplicity = multiplicity;
	outVertex3.position = vec4(c, 1.0);
	
	outVertices[idx] = outVertex1;
	outVertices[idx + 1] = outVertex2;
	outVertices[idx + 2] = outVertex3;
}

void main()
{
	uint triangleIdx = gl_GlobalInvocationID.x;

	if (triangleIdx * 3 < indexCount)
	{
		uint firstIdx = indexOffset + triangleIdx * 3;
		
		uint indices[3];
		InVertex vertices[3];
		for (uint i = 0; i < 3; i++)
		{
			indices[i] = inIndices[firstIdx + i];
			vertices[i] = inVertices[indices[i]];
		}
		
		generateShadowVolume(
			triangleIdx,
			indexCount,
			extrusionDistance,
			lightDir,
			vertices,
			indices);
	}
}
