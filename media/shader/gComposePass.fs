#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gColor;
layout (location = 3) out vec4 outDsPosition;
layout (location = 4) out vec4 outDsNormal;
layout (location = 5) out vec4 outDsColor;

uniform sampler2D inPosition;
uniform sampler2D inNormal;
uniform sampler2D inColor;
uniform sampler2D dsPosition;
uniform sampler2D dsNormal;
uniform sampler2D dsColor;

uniform float bgRoughness = 0.5;
uniform float bgMetallic = 0.5;

void main()
{
	vec4 position = texture(inPosition, TexCoord);
    vec4 normal = texture(inNormal, TexCoord);
    vec4 color = texture(inColor, TexCoord);
	
	// Depth sensor outputs
	vec3 dsposition = texture(dsPosition, TexCoord).rgb;
	vec3 dsnormal = texture(dsNormal, TexCoord).rgb;
	vec4 dscolor = texture(dsColor, TexCoord);
	dscolor.a = 0;
	
	// Mix gBuffer and kinect position and color
	vec4 mixPosition = position;
	vec4 mixNormal = normal;
	vec4 mixColor = color;
	
	if (mixPosition.z < dsposition.z)
	{
		mixPosition.xyz = dsposition;
		mixNormal.xyz = dsnormal;
		mixColor = vec4(dscolor.rgb, 0);
	}
	
	if (mixColor.a == 0)
	{
		mixPosition = vec4(dsposition, bgRoughness);
		mixNormal = vec4(dsnormal, bgMetallic);
		mixColor.rgb = dscolor.rgb;
	}
	
	// Convert colors to linear space
	mixColor.rgb = pow(mixColor.rgb, vec3(2.2));
	dscolor.rgb = pow(dscolor.rgb, vec3(2.2));
	
	gPosition = mixPosition;
	gNormal = mixNormal;
	gColor = mixColor;
	
	outDsPosition = vec4(dsposition, bgRoughness);
	outDsNormal = vec4(dsnormal, bgMetallic);
	outDsColor = dscolor;
}