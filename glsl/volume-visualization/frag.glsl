#version 330

uniform vec3 lightDir;

in VertexOutput
{
	vec4 position;
	flat int multiplicity;
} IN;

out vec4 outColor;

void main()
{
	outColor = gl_FrontFacing ? vec4(0.0, 1.0, 0.0, 1.0) : vec4(0.0, 0.0, 1.0, 1.0);
}
