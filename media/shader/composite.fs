#version 450

in vec2 TexCoord;

out vec4 outColor;

uniform sampler2D fullScene;
uniform sampler2D backScene;
uniform sampler2D dsColor;

float rgb2gray(vec3 color)
{
	color = vec3(0.2989, 0.5870, 0.1140) * color;
	return color.r + color.b + color.g;
}

void main()
{
	vec4 fullcolor = texture(fullScene, TexCoord);
	vec4 backcolor = texture(backScene, TexCoord);
	vec3 dscolor = texture(dsColor, TexCoord).rgb;
	
	vec3 finalcolor = fullcolor.rgb * fullcolor.a + (dscolor + fullcolor.rgb - backcolor.rgb) * (1 - fullcolor.a);
	
	outColor = vec4(finalcolor, 1);
}