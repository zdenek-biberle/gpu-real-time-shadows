#version 430

uniform mat4 pMat;
uniform mat4 mvMat;


layout(location = 0) in vec4 inPosition;
layout(location = 1) in int multiplicity;

out VertexOutput
{
	vec4 position;
	int multiplicity;

} OUT;

void main()
{
	OUT.position = mvMat * inPosition;
	gl_Position = pMat * OUT.position;
	OUT.multiplicity = multiplicity;
}
