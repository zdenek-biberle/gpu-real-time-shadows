#version 430

uniform vec3 lightDir;
layout(binding=0) uniform isampler2D stencilTexture;  

in VertexOutput
{
	vec4 position;
	vec3 normal;
} IN;

out vec4 outColor;

void main()
{
	int number = texture(stencilTexture, gl_FragCoord.xy).r;	//here look into texture with shadowing info.. zeroes should be lighted
	vec3 ambient = vec3(0.3f);

	if(number == 0){
		vec3 normNormal = normalize(IN.normal);
	
		vec3 diffuse = vec3(max(0.0, dot(normNormal, -lightDir)));
	
		vec3 light = diffuse + ambient;
	
		outColor.z = 1.0;
		outColor.xyz = vec3(0.5, 0.5, 1.0) * light;

	} else if(number == 1){
		vec3 normNormal = normalize(IN.normal);
	
		vec3 diffuse = vec3(max(0.0, dot(normNormal, -lightDir)));
	
		vec3 light = diffuse + ambient;
	
		outColor.z = 1.0;
		outColor.xyz = vec3(1.0, 0.5, 0.0) * light;

	} else if(number == -1){
		vec3 normNormal = normalize(IN.normal);
	
		vec3 diffuse = vec3(max(0.0, dot(normNormal, -lightDir)));
	
		vec3 light = diffuse + ambient;
	
		outColor.z = 1.0;
		outColor.xyz = vec3(0.0, 1.0, 0.0) * light;

	} else { 
		outColor.z = 1.0;
		outColor.xyz = ambient;		
	}
}


