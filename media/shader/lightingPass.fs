#version 450

in vec2 Texcoord;

out vec4 outColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColor;

void main()
{
	vec3 FragPos = texture(gPosition, Texcoord).rgb;
    vec3 Normal = texture(gNormal, Texcoord).rgb;
    vec3 Albedo = texture(gColor, Texcoord).rgb;
    float Specular = texture(gColor, Texcoord).a;
	
	outColor = vec4(Albedo, 1.0);
}