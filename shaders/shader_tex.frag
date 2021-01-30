#version 430 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform sampler2D textureSampler;

uniform vec3 lightPosCube;

in vec3 FragPos;
in vec3 interpNormal;
in vec2 interpTexCoord;

void main()
{
	vec2 modifiedTexCoord = vec2(interpTexCoord.x, 1.0 - interpTexCoord.y); // Poprawka dla tekstur Ziemi, ktore bez tego wyswietlaja sie 'do gory nogami'
	vec3 color = texture2D(textureSampler, modifiedTexCoord).rgb;
	
	//ambient

	float ambientStrength = 0.1;
    vec3 ambient = vec3(ambientStrength,ambientStrength,ambientStrength);

	//slonce

	vec3 norm = normalize(interpNormal);
	vec3 lightDir = normalize(vec3(0,0,0) - FragPos); 

	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = vec3(diff,diff,diff);

	//cube

	vec3 norm2 = normalize(interpNormal);
	vec3 lightDir2 = normalize(lightPosCube - FragPos); 

	float diff2 = max(dot(norm2, lightDir2), 0.0);
	vec3 diffuse2 = vec3(diff2,diff2,diff2);

	//all

	vec3 result = (ambient + diffuse*0.5 + diffuse2*0.5) * color;

	FragColor = vec4(result, 1.0);
	BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
