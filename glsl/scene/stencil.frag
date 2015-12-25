#version 450

layout(early_fragment_tests) in;

in VertexOutput
{
	flat int multiplicity;
} IN;

layout(binding = 0, R32I) uniform iimage2D stencilTexture;	// r32i to support   int  imageAtomicAdd(IMAGE_INFO, int data);

void main()
{
	int m;

	if(gl_FrontFacing == true){
		m = IN.multiplicity;
	} else {
		m = -IN.multiplicity;
	}

	imageAtomicAdd(stencilTexture, ivec2(gl_FragCoord.xy), m);
}
