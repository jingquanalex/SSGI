#version 450

in vec2 TexCoord;

out vec4 outColor;

uniform sampler2D fullScene;
uniform sampler2D planeScene;
uniform sampler2D dsColor;

void main()
{
	//outColor = vec4(0.9, 0.7, 0.2, 1.0);
	vec4 fullcolor = texture(fullScene, TexCoord);
	vec4 planecolor = texture(planeScene, TexCoord);
	vec2 dsTexCoord = vec2(TexCoord.x, 1.0 - TexCoord.y);
	vec4 dscolor = texture(dsColor, dsTexCoord);
	
	vec4 finalcolor = dscolor;
	
	if (planecolor.a == 0)
	{
		planecolor.rgb = vec3(0);
	}
	
	if (fullcolor.a == 1.0)
	{
		finalcolor.rgb = fullcolor.rgb;
	}
	
	if (fullcolor.a > 0.0 && fullcolor.a < 1.0)
	{
		finalcolor.rgb = dscolor.rgb - abs(fullcolor.rgb - planecolor.rgb) * 2;
	}
	
	outColor = finalcolor;
}