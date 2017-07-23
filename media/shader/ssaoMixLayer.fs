#version 450

in vec2 TexCoord;

layout (location = 0) out vec3 outColor;

uniform sampler2D layer1;
uniform sampler2D layer2;
uniform sampler2D gColor;

void main()
{
	vec3 color1 = texture(layer1, TexCoord).rgb;
    vec3 color2 = texture(layer2, TexCoord).rgb;
	float mask = texture(gColor, TexCoord).a;
	
	vec3 mixColor = color1;
	if (color2.r < color1.r)
	{
		mixColor = color2;
	}
	
	if (mask == 1)
	{
		mixColor = color1;
	}
	
	outColor = mixColor;
}