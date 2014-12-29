#version 330

uniform vec3 lightDir;

in VertexOutput
{
	vec4 position;
	vec3 normal;
} IN;

out vec4 outColor;

void main()
{
	vec3 normNormal = normalize(IN.normal);
	
	vec3 ambient = vec3(0.3);
	vec3 diffuse = vec3(max(0.0, dot(normNormal, -lightDir)));
	
	vec3 light = ambient + diffuse;
	
	outColor.z = 1.0;
	outColor.xyz = vec3(0.5, 0.5, 1.0) * light;
}
