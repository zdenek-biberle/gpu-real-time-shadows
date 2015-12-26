#version 450

uniform mat4 pMat;
uniform mat4 mvMat;
uniform mat3 mvNormMat;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;

out VertexOutput
{
	vec4 position;
	vec3 normal;
} OUT;

void main()
{
	OUT.position = mvMat * inPosition;
	OUT.normal = normalize(mat3(mvNormMat) * inNormal);
	
	gl_Position = pMat * OUT.position;
}
