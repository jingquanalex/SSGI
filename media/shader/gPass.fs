#version 450

in vec3 Position;
in vec3 Normal;
in vec2 Texcoord;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gColor;

uniform sampler2D diffuse1;
uniform sampler2D normal1;
uniform sampler2D specular1;

void main()
{
	gPosition = Position;
	gNormal = normalize(Normal);
	gColor = texture(diffuse1, Texcoord);
	//gColor.a = texture(specular1, Texcoord).a;
}