#version 430

in VertexOutput
{
	flat int multiplicity;
} IN;

layout(r32i) uniform iimage2D stencilTexture;	// r32i to support   int  imageAtomicAdd(IMAGE_INFO, int data);

void main()
{

	int m;

	if(gl_FrontFacing == true){
		m = IN.multiplicity;
	} else {
		m = -IN.multiplicity;
	}

	int value = imageAtomicAdd(stencilTexture, ivec2(gl_FragCoord.xy), m);

}
