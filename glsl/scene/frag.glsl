#version 330

uniform vec3 lightDir;
layout(binding=0) uniform sampler2D stencilTexture;  //

in VertexOutput
{
	vec4 position;
	vec3 normal;
} IN;

out vec4 outColor;

void main()
{
	float number = texture(stencilTexture, gl_FragCoord.xy).x;	//here look into texture with shadowing info.. zeroes should be lighted

	//if(number < 0.01f && number > -0.01f){
		vec3 normNormal = normalize(IN.normal);
	
		vec3 diffuse = vec3(max(0.0, dot(normNormal, -lightDir)));
	
		vec3 ambient = vec3(0.3);
		vec3 light = diffuse + ambient;
	
		outColor.z = 1.0;
		outColor.xyz = vec3(0.5, 0.5, 1.0) * light;
/*
	} else {
		outColor.z = 1.0;
		outColor.xyz = vec3(0.0, 0.0, 0.0);		
	}*/
}
