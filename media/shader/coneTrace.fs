#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 outColor;

uniform sampler2D inColor;
uniform sampler2D inLight;


void main()
{
	vec4 color = texture(inColor, TexCoord);
	vec4 light = textureLod(inLight, TexCoord, 1);
	//float lightMask = textureLod(inLight, TexCoord, 0).a;
	
	vec3 finalColor = mix(color.rgb, light.rgb, light.a);
	finalColor = light.rgb;
	//finalColor = color.rgb;
	
	outColor = vec4(finalColor, color.a);
}