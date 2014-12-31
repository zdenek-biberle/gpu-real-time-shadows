#version 430

layout(early_fragment_tests) in;

in VertexOutput
{
	flat int multiplicity;
} IN;

layout(r32i, binding = 0) uniform iimage2D stencilTexture;	// r32i to support   int  imageAtomicAdd(IMAGE_INFO, int data);

out vec4 dummy;

void main()
{

	int m;

	if(gl_FrontFacing == true){
		m = IN.multiplicity;
	} else {
		m = -IN.multiplicity;
	}

	imageAtomicAdd(stencilTexture, ivec2(gl_FragCoord.xy), m);
	
	if (m == 0)
		dummy = vec4(1.0, 1.0, 1.0, 1.0);
	else if (m == 1)
		dummy = vec4(1.0, 1.0, 0.0, 1.0);
	else if (m == -1)
		dummy = vec4(1.0, 0.0, 1.0, 1.0);
	else if (m == 2)
		dummy = vec4(1.0, 0.0, 0.0, 1.0);
	else if (m == -2)
		dummy = vec4(0.0, 1.0, 1.0, 1.0);
	else if (m == 3)
		dummy = vec4(0.0, 1.0, 0.0, 1.0);
	else if (m == -3)
		dummy = vec4(0.0, 0.0, 1.0, 1.0);
	else
		dummy = vec4(1.0, 0.5, 0.5, 1.0);
}
