#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3 * 10) out; // 3 * 8 -> maximalne osm trianglu

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

uniform uint indexCount;
uniform float extrusionDistance = 100.0;
uniform vec3 lightPos;

in VertexOutput
{
	int vertexIdx;
} IN[];

out GeometryOutput
{
	flat int multiplicity;
} OUT;

void generateShadowVolume(
	uint triangleIdx,
	uint indexCount,
	float extrusionDistance,
	vec3 lightPos, 
	InVertex vertices[3], 
	uint indices[3]);

uint reserveTriangles(uint n)
{
	return 0; // geometry shader doesn't write to a buffer -> we don't care about the id
}

void emitTriangle(uint idx, vec3 a, vec3 b, vec3 c, int multiplicity)
{
	gl_Position = pMat * mvMat * vec4(a, 1.0);
	OUT.multiplicity = multiplicity;
	EmitVertex();
	gl_Position = pMat * mvMat * vec4(b, 1.0);
	OUT.multiplicity = multiplicity;
	EmitVertex();
	gl_Position = pMat * mvMat * vec4(c, 1.0);
	OUT.multiplicity = multiplicity;
	EmitVertex();
	EndPrimitive();
}
	
void main()
{
	uint indices[3];
	InVertex vertices[3];
	for (uint i = 0; i < 3; i++)
	{
		indices[i] = IN[i].vertexIdx;
		vertices[i] = inVertices[indices[i]];
	}
	
	generateShadowVolume(
		gl_PrimitiveIDIn,
		indexCount,
		extrusionDistance,
		lightPos,
		vertices,
		indices);
}