#version 450

uniform vec3 lightDir;
//layout(binding = 0, R32I) uniform iimage2D stencilTexture;
uniform vec3 color = vec3(0.8, 0.2, 0.1);
uniform float shadowCasterOpacity = 0.25;
in VertexOutput
{
	vec4 position;
	vec3 normal;
} IN;

out vec4 outColor;

void main()
{
	// selfshadowing looks a bit weird, why?
	//int number = imageLoad(stencilTexture, ivec2(gl_FragCoord.xy)).r;
	vec3 ambient = vec3(0.32, 0.3, 0.25);
	
	vec3 normNormal = normalize(IN.normal);
	vec3 diffuse = vec3(max(0.0, dot(normNormal, -lightDir)));
	vec3 light = diffuse + ambient;

	outColor = vec4(color * light, shadowCasterOpacity);
}


