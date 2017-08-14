#version 450

in vec3 Position;
in vec2 TexCoord;
in vec3 Normal;

layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gColor;

uniform sampler2D diffuse1;
uniform sampler2D normal1;
uniform sampler2D specular1;

uniform float roughness = 0.0;
uniform float metallic = 0.0;

void main()
{
	vec3 position = Position;
	vec4 color = texture(diffuse1, TexCoord);
	
	gPosition = vec4(position, roughness);
	gNormal = vec4(normalize(Normal), metallic);
	gColor = color;
}