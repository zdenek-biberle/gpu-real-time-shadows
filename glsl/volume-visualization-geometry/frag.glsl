#version 450

in VertexOutput
{
	flat int multiplicity;
} IN;

out vec4 outColor;

void main()
{
	float multiplicityVisual = (IN.multiplicity * 0.125) + 0.5;
	bool negativeMultiplicity = IN.multiplicity < 0;
	bool side = gl_FrontFacing ? negativeMultiplicity : !negativeMultiplicity;
	outColor = side ? vec4(0.8, 0.8, 0.8, 0.5) : vec4(0.8, 0.2, 0.2, 0.5);
}
