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
	float multiplicityVisual = (IN.multiplicity * 0.125) + 0.5;
	outColor = (gl_FrontFacing ? vec4(0.5, multiplicityVisual, 0, 0.2) : vec4(multiplicityVisual, multiplicityVisual, multiplicityVisual, 0.2));
}
