#version 450

in vec2 Texcoord;

out vec4 outColor;

uniform sampler2D diffuse1;
uniform sampler2D normal1;
uniform sampler2D specular1;

void main()
{
	//outColor = vec4(0.9, 0.7, 0.2, 1.0);
	outColor = texture(diffuse1, Texcoord);
}