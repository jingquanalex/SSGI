#version 450

in vec2 TexCoord;

out vec4 outColor;

uniform sampler2D fullScene;
uniform sampler2D backScene;
uniform sampler2D dsColor;

void main()
{
	vec4 fullcolor = texture(fullScene, TexCoord);
	vec4 backcolor = texture(backScene, TexCoord);
	vec3 dscolor = texture(dsColor, TexCoord).rgb;
	
	vec4 finalcolor = vec4(dscolor, 1);
	
	if (fullcolor.a == 1.0)
	{
		finalcolor.rgb = fullcolor.rgb;
	}
	else
	{
		vec3 fullcolorMasked = fullcolor.rgb * (1 - fullcolor.a);
		finalcolor.rgb = dscolor - abs(fullcolorMasked - backcolor.rgb);
	}
	
	outColor = vec4(finalcolor);
}