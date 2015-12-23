#version 450

uniform vec3 lightDir;
layout(binding = 0, r32i) uniform readonly iimage2D stencilTexture;

in VertexOutput
{
	vec4 position;
	vec3 normal;
} IN;

out vec4 outColor;

void main()
{
	int number = imageLoad(stencilTexture, ivec2(gl_FragCoord.xy)).r;	//here look into texture with shadowing info.. zeroes should be lighted
	vec3 ambient = vec3(0.32, 0.3, 0.25);
	
	vec3 normNormal = normalize(IN.normal);
	vec3 diffuse = vec3(max(0.0, dot(normNormal, -lightDir)));
	vec3 light = (number > 0 ? 0 : 1) * diffuse + ambient;

	outColor.z = 1.0;
	outColor.xyz = vec3(0.5, 0.5, 1.0) * light;
}


