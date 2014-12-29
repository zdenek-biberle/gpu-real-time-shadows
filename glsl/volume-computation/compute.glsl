#version 430

layout (local_size_x = 128) in;

uniform uint indexCount;
uniform uint indexOffset;
uniform vec3 lightDir = vec3(-1.0, -1.0, -1.0);

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
	uint padding0;
	uint padding1;
};

layout (std430, binding = 0) readonly buffer InVertices
{
	InVertex inVertices[];
};

layout (std430, binding = 1) readonly buffer InIndices
{
	uint inIndices[];
};

layout (std430, binding = 2) writeonly buffer OutVertices
{
	OutVertex outVertices[];
};

layout (std430, binding = 3) writeonly buffer OutInfo
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
	return dot(normalize(n), normalize(lightDir)) > 0;
	//return a.x > 0 && b.x > 0 && c.x > 0;
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
		
		uint aidx[3];
		aidx[0] = inIndices[firstIdx];
		aidx[1] = inIndices[firstIdx + 1];
		aidx[2] = inIndices[firstIdx + 2];
		vec3 a0 = position(inVertices[aidx[0]]);
		vec3 a1 = position(inVertices[aidx[1]]);
		vec3 a2 = position(inVertices[aidx[2]]);
		
		bool thisFrontFacing = isFrontFacing(a0, a1, a2);
		
		if (thisFrontFacing)
		{
			emitTriangle(a0, a1, a2, 0, 1);
		}
		
		uint edgeIndices[] = {aidx[0], aidx[1], aidx[1], aidx[2], aidx[2], aidx[0]};
		uint edgeMultiplicity[] = {0, 0, 0};
		
		for (uint i = 0; i < indexCount; i += 3)
		{
			if (i != triangleId * 3)
			{
				for (uint edgeIdx = 0; edgeIdx < 3; edgeIdx++)
				{
					uint thisEdge[2];
					thisEdge[0] = edgeIndices[i * 2];
					thisEdge[1] = edgeIndices[i * 2 + 1];
					
					uint bidx[3];
					bidx[0] = inIndices[i + indexOffset];
					bidx[1] = inIndices[i + 1 + indexOffset];
					bidx[2] = inIndices[i + 2 + indexOffset];
				
					uint matchingVertices = 0;
					for (uint edgeVertIdx = 0; edgeVertIdx < 2; edgeVertIdx++)
					for (uint otherVertIdx = 0; otherVertIdx < 3; otherVertIdx++)
					{
						if (thisEdge[edgeVertIdx] == bidx[otherVertIdx]) matchingVertices++;
					}
					
					if (matchingVertices >= 2)
					{
						// kazdou hranu zpracovava trojuhelnik s nejnizsim indexem
						if (i < triangleId * 3) break;
						
						vec3 b0 = position(inVertices[bidx[0]]);
						vec3 b1 = position(inVertices[bidx[1]]);
						vec3 b2 = position(inVertices[bidx[2]]);
					
						if (isFrontFacing(b0, b1, b2) != thisFrontFacing)
						
					}
				}
			}
		}
	}
}
