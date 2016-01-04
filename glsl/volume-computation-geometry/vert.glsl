#version 450

layout(location = 0) in vec4 inPosition;

out VertexOutput
{
	int vertexIdx;
} OUT;

void main()
{
	OUT.vertexIdx = gl_VertexID;
	gl_Position = inPosition;
}
