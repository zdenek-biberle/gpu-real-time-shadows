#version 330

uniform mat4 pMat;
uniform mat4 mvMat;
uniform mat3 mvNormMat;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in int inMultiplicity;

out VertexOutput
{
	vec4 position;
	flat int multiplicity;
} OUT;

void main()
{
	OUT.position = mvMat * inPosition;
	OUT.multiplicity = inMultiplicity;
	
	gl_Position = pMat * OUT.position;
}
