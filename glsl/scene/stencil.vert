#version 430

uniform mat4 pMat;
uniform mat4 mvMat;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in int multiplicity;

out VertexOutput
{
	flat int multiplicity;
} OUT;

void main()
{
	vec4 position = mvMat * inPosition;
	gl_Position = pMat * position;
	OUT.multiplicity = multiplicity;
}
