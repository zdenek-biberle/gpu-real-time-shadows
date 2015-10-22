#version 330

 //gl_Position: the clip-space position of the vertex.



 
layout(std140) uniform GlobalMatrices
{
	mat4 cameraToClipMatrix;	//perspective
	mat4 worldToCameraMatrix;  //position camera
};


layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inCoordinates;

uniform mat4 objectTransform;	

out vec2 textureCoordinates; // Interpolated values from the vertex shaders

void main()
{

	gl_Position = cameraToClipMatrix * worldToCameraMatrix * objectTransform * vec4(inPosition, 0.0, 1.0);
	
	textureCoordinates = inCoordinates;
}
