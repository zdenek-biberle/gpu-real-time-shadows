#version 330

uniform mat4 pMat;
uniform mat4 mvMat;
uniform mat3 mvNormMat;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

out VertexOutput
{
	vec4 position;
	vec2 texCoord;
	vec3 normal;
} OUT;

void main()
{
	OUT.position = mvMat * inPosition;
	OUT.texCoord = inTexCoord;
	OUT.normal = normalize(mat3(mvNormMat) * inNormal);
	
	gl_Position = pMat * OUT.position;
}
