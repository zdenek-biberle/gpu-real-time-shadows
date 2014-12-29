#version 430


in VertexOutput
{
	vec4 position;

} IN;

out vec4 outColor;

void main()
{

	vec3 ambient = vec3(0.3);

	
	vec3 light = ambient;    //so i dont have to rewrite drawing right now..
	
	outColor.z = 1.0;
	outColor.rgb = vec3(0.5, 0.5, 1.0) * light;
}
