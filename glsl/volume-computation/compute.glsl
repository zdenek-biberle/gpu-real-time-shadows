#version 430

layout (local_size_x = 128) in;

uniform uint indexCount;
uniform uint indexOffset;
uniform vec3 lightDir = vec3(-1.0, -1.0, -1.0);
uniform float extrusionDistance = 100.0;

struct InVertex
{
	float x;
	float y;
	float z;
	//int padding;
};

struct OutVertex
{
	vec4 position;
	int multiplicity;
	uint isCap;
	//uint padding0;
	//uint padding1;
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

layout (std430, binding = 2) writeonly buffer OutVertices
{
	OutVertex outVertices[];
};

layout (std430, binding = 3) writeonly buffer OutInfo
{
	uint outTriCount;
};

layout (std430, binding = 4) readonly buffer EdgeLookupBuffer
{
	EdgeLookupNode edgeLookup[];
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
	return dot(normalize(n), normalize(lightDir)) < 0;
	//return a.x > 0 && b.x > 0 && c.x > 0;
}

// Rezervuje trojuhelniky pro emitTriangle. Vhodne pro n > 1.
uint reserveTriangles(uint n)
{
	return atomicAdd(outTriCount, n);
}

void emitTriangle(uint idx, vec3 a, vec3 b, vec3 c, int multiplicity, uint isCap)
{
	idx *= 3;
	
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

// zjisti, zda je bod point pred nebo za rovinou definovanou body a, b a c
bool isInFront(vec3 point, vec3 a, vec3 b, vec3 c)
{
	vec3 ab = b - a;
	vec3 ac = c - a;
	
	vec3 normal = normalize(cross(ab, ac));
	vec3 pointvec = normalize(point - a);
	return dot(pointvec, normal) > 0.0;
}

int edgeLookupNodeCompare(EdgeLookupNode node, uint edge0, uint edge1)
{
	if (node.idx0 < edge0) return -1;
	else if (node.idx0 > edge0) return 1;
	else if (node.idx1 < edge1) return -1;
	else if (node.idx1 > edge1) return 1;
	else return 0;
}

bool hasEdge(EdgeLookupNode node, uint edge0, uint edge1)
{
	return (node.idx0 == edge0 && node.idx1 == edge1)
		|| (node.idx0 == edge1 && node.idx1 == edge0);
}

// vrati id prvniho prvku v poli edgeLookup, ktery je na stejne hrane
uint doEdgeLookup(uint edge0, uint edge1)
{
	if (edge0 < edge1)
	{
		uint tmp = edge0;
		edge0 = edge1;
		edge1 = tmp;
	}
	
	uint loIdx = 0;
	uint hiIdx = indexCount;
	uint midIdx;
	EdgeLookupNode node;
	
	// binarne vyhledame shodny prvek
	while(true)
	{
		midIdx = (loIdx + hiIdx) / 2;
		node = edgeLookup[midIdx];
		int comparison = edgeLookupNodeCompare(node, edge0, edge1);
		if (comparison == -1) loIdx = midIdx;
		else if (comparison == 1) hiIdx = midIdx;
		else break;
	}
	
	while (midIdx > 0)
	{
		// pokusime se najit nejnizsi shodny prvek
		node = edgeLookup[midIdx - 1];
		if (node.idx0 == edge0 && node.idx1 == edge1)
		{
			midIdx--;
		}
		else break;
	}
	
	return midIdx;
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
		
		vec3 extrusionVec = extrusionDistance * normalize(lightDir);
		
		if (isFrontFacing(a0, a1, a2))
		{
			uint triIdx = reserveTriangles(2);
			emitTriangle(triIdx, a0, a1, a2, -2, 1);
			emitTriangle(triIdx + 1, a0 + extrusionVec, a2 + extrusionVec, a1 + extrusionVec, -2, 1);	
		}
		
		uint edgeIndices[] = {aidx[0], aidx[1], aidx[1], aidx[2], aidx[2], aidx[0]};
		
		for (uint edgeIdx = 0; edgeIdx < 3; edgeIdx++)
		{
			uint thisEdge[2];
			thisEdge[0] = edgeIndices[edgeIdx * 2];
			thisEdge[1] = edgeIndices[edgeIdx * 2 + 1];
			int edgeMultiplicity = 0;
			bool ignore = false;
			uint edgeNode;
			for (edgeNode = doEdgeLookup(thisEdge[0], thisEdge[1]);
				edgeNode < indexCount && hasEdge(edgeLookup[edgeNode], thisEdge[0], thisEdge[1]);
				edgeNode++)
			{
				EdgeLookupNode node = edgeLookup[edgeNode];
				
				// kazdou hranu zpracovava trojuhelnik s nejnizsim indexem
				if (node.triangleIdx < triangleId)
				{
					ignore = true;
					break;
				}
				
				vec3 edge0 = position(inVertices[thisEdge[0]]);
				vec3 edge1 = position(inVertices[thisEdge[1]]);
				vec3 thirdVert = position(inVertices[node.idx2]);
			
				edgeMultiplicity += isInFront(thirdVert, edge0, edge1, edge1 + lightDir) ? -1 : 1;
			}
			
			if (!ignore && edgeMultiplicity != 0)
			{
				vec3 edge0 = position(inVertices[edgeIndices[edgeIdx * 2]]);
				vec3 edge1 = position(inVertices[edgeIndices[edgeIdx * 2 + 1]]);
				
				uint triIdx = reserveTriangles(2);
				emitTriangle(triIdx, edge0, edge1, edge0 + extrusionVec, edgeMultiplicity, 0);
				emitTriangle(triIdx + 1, edge1, edge1 + extrusionVec, edge0 + extrusionVec, edgeMultiplicity, 0);
			}
		}
	}
}
