#version 430

layout (local_size_x = 128) in;

uniform uint indexCount;
uniform uint indexOffset;
uniform vec3 lightDir = vec3(1.0, 1.0, 1.0);

struct InVertex
{
	float x;
	float y;
	float z;
	float nx;
	float ny;
	float nz;
	float u;
	float v;
};

struct OutVertex
{
	vec4 position;
	uint multiplicity;
	uint isCap;
	uint padding[2];
};

layout (std430, binding = 0) buffer InVertices
{
	InVertex inVertices[];
};

layout (std430, binding = 1) buffer InIndices
{
	uint inIndices[];
};

layout (std430, binding = 2) buffer OutVertices
{
	OutVertex outVertices[];
};

layout (std430, binding = 3) buffer OutInfo
{
	uint outVertCount;
};

vec3 position(InVertex vert)
{
	return vec3(vert.x, vert.y, vert.z);
}

bool isFrontFacing(vec3 a, vec3 b, vec3 c)
{
	vec3 ab = b - a;
	vec3 ac = c - a;
	vec3 n = cross(ab, ac);
	return a.x > 0 && b.x > 0 && c.x > 0;
}

void emitTriangle(vec3 a, vec3 b, vec3 c, uint multiplicity, uint isCap)
{
	uint idx = atomicAdd(outVertCount, 3);
	OutVertex outVertex;
	outVertex.multiplicity = multiplicity;
	outVertex.isCap = isCap;
	
	outVertex.position = vec4(a, 1.0);
	outVertices[idx] = outVertex;
	outVertex.position = vec4(b, 1.0);
	outVertices[idx + 1] = outVertex;
	outVertex.position = vec4(c, 1.0);
	outVertices[idx + 2] = outVertex;
}

void main()
{
	uint triangleId = gl_GlobalInvocationID.x;
	if (triangleId * 3 < indexCount)
	{
		uint firstIdx = indexOffset + triangleId * 3;
		vec3 a = position(inVertices[inIndices[firstIdx]]);
		vec3 b = position(inVertices[inIndices[firstIdx + 1]]);
		vec3 c = position(inVertices[inIndices[firstIdx + 2]]);
		
		if (true||isFrontFacing(a, b, c)) // true kvuli debugu
		{
			emitTriangle(a, b, c, 0, 1);
		}
	}
}
