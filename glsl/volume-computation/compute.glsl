#version 450

layout (local_size_x = 128) in;

uniform uint indexCount;
uniform uint indexOffset;
uniform float extrusionDistance = 100.0;
uniform vec3 lightPos;

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
	return dot(normalize(n), -normalize(lightPos)) < 0;
	//return a.x > 0 && b.x > 0 && c.x > 0;
}

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
	outVertex2.multiplicity = multiplicity;
	outVertex3.multiplicity = multiplicity;

	
	
	outVertex1.position = vec4(a, 1.0);
	outVertices[idx] = outVertex1;

	outVertex2.position = vec4(b, 1.0);
	outVertices[idx + 1] = outVertex2;

	outVertex3.position = vec4(c, 1.0);
	outVertices[idx + 2] = outVertex3;
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
	vec3 lightDir = -normalize(lightPos);

	if (triangleId * 3 < indexCount)
	{
		uint firstIdx = indexOffset + triangleId * 3;
		
		uint aidx[3];
		aidx[0] = inIndices[firstIdx];
		aidx[1] = inIndices[firstIdx + 1];
		aidx[2] = inIndices[firstIdx + 2];

		InVertex vert1a = inVertices[aidx[0]];
		InVertex vert2a = inVertices[aidx[1]];
		InVertex vert3a = inVertices[aidx[2]];


		vec3 a0 = position(vert1a);
		vec3 a1 = position(vert2a);
		vec3 a2 = position(vert3a);
		
		
		if (isFrontFacing(a0, a1, a2))
		{
			uint triIdx = reserveTriangles(1);	//returns triCount before adding the number.. effectively it's index of next free triangle "slot" in array
			//emitTriangle(triIdx, a0, a1, a2, -2, 1);
			emitTriangle(triIdx, a0 + normalize(a0 - lightPos) * extrusionDistance, a1 + normalize(a1 - lightPos) * extrusionDistance, a2 + normalize(a2 - lightPos) * extrusionDistance, -1);	
		}
		
		uint edgeIndices[] = {aidx[0], aidx[1], aidx[1], aidx[2], aidx[2], aidx[0]};
		
		for (uint edgeIdx = 0; edgeIdx < 3; edgeIdx++)
		{
			uint thisEdge[2];
			thisEdge[0] = edgeIndices[edgeIdx * 2];
			thisEdge[1] = edgeIndices[edgeIdx * 2 + 1];
			int edgeMultiplicity = 0;
			bool ignore = false;

			//attempt at fix
			uint edgeNode = doEdgeLookup(thisEdge[0], thisEdge[1]);
			EdgeLookupNode node = edgeLookup[edgeNode];
			bool hasEdge_bool = hasEdge(node, thisEdge[0], thisEdge[1]);

			for (edgeNode; edgeNode < indexCount && hasEdge_bool; )
			{
				//EdgeLookupNode node = edgeLookup[edgeNode];
				
				// kazdou hranu zpracovava trojuhelnik s nejnizsim indexem
				if (node.triangleIdx < triangleId)
				{
					ignore = true;
					break;
				}
				
				InVertex vert1b = inVertices[thisEdge[0]];
				InVertex vert2b = inVertices[thisEdge[1]];
				InVertex vert3b = inVertices[node.idx2];

				vec3 edge0 = position(vert1b);
				vec3 edge1 = position(vert2b);
				vec3 thirdVert = position(vert3b);
			
				edgeMultiplicity += isInFront(thirdVert, edge0, edge1, edge1 + lightDir) ? -1 : 1;

				//this was originally in the for loop
				edgeNode++;
				node = edgeLookup[edgeNode];
				hasEdge_bool = hasEdge(node, thisEdge[0], thisEdge[1]);
			}
			
			if (!ignore && edgeMultiplicity != 0)
			{

				uint edge1x = edgeIndices[edgeIdx * 2];
				uint edge2x = edgeIndices[edgeIdx * 2 + 1];
				InVertex vert1c = inVertices[edge1x];
				InVertex vert2c = inVertices[edge2x];

				vec3 edge0 = position(vert1c);
				vec3 edge1 = position(vert2c);
				
				uint triIdx = reserveTriangles(2);
				emitTriangle(triIdx, edge0, edge1, edge0 + normalize(edge0 - lightPos) * extrusionDistance, edgeMultiplicity);
				emitTriangle(triIdx + 1, edge1, edge1 + normalize(edge1 - lightPos) * extrusionDistance, edge0 + normalize(edge0 - lightPos) * extrusionDistance, edgeMultiplicity);
			}
		}
	}
}
