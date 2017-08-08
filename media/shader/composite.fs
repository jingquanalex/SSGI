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
	vec4 fullcolor = clamp(texture(fullScene, TexCoord), 0.0, 1.0);
	vec4 backcolor = clamp(texture(backScene, TexCoord), 0.0, 1.0);
	vec3 dscolor = texture(dsColor, TexCoord).rgb;
	vec4 finalcolor = fullcolor;
	
	vec3 objects = fullcolor.rgb * fullcolor.a;
	//vec3 background = (dscolor * fullcolor.rgb / backcolor.rgb) * (1 - fullcolor.a);
	vec3 background = (dscolor + fullcolor.rgb - backcolor.rgb) * (1 - fullcolor.a);
	finalcolor.rgb = objects + clamp(background, 0.0, 1.0);
	//finalcolor.rgb = dscolor + fullcolor.rgb - backcolor.rgb;

	outColor = finalcolor;
}