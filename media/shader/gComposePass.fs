#version 450

in vec2 TexCoord;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gColor;
layout (location = 3) out vec3 outDsPosition;
layout (location = 4) out vec3 outDsNormal;
layout (location = 5) out vec4 outDsColor;

uniform sampler2D inPosition;
uniform sampler2D inNormal;
uniform sampler2D inColor;
uniform sampler2D dsPosition;
uniform sampler2D dsNormal;
uniform sampler2D dsColor;


void main()
{
	vec3 position = texture(inPosition, TexCoord).xyz;
    vec3 normal = texture(inNormal, TexCoord).xyz;
    vec4 color = texture(inColor, TexCoord);
	
	// Depth sensor outputs
	vec3 dsposition = texture(dsPosition, TexCoord).rgb;
	vec3 dsnormal = texture(dsNormal, TexCoord).rgb;
	vec4 dscolor = texture(dsColor, TexCoord);
	dscolor.a = 0;
	
	// Mix gBuffer and kinect position and color
	vec3 mixPosition = position;
	vec3 mixNormal = normal;
	vec4 mixColor = color;
	
	if (mixPosition.z < dsposition.z)
	{
		mixPosition = dsposition;
		mixNormal = dsnormal;
		mixColor = vec4(dscolor.rgb, 0);
	}
	
	if (mixColor.a == 0)
	{
		mixPosition = dsposition;
		mixNormal = dsnormal;
		mixColor.rgb = dscolor.rgb;
	}
	
	// Convert colors to linear space
	mixColor.rgb = pow(mixColor.rgb, vec3(2.2));
	dscolor.rgb = pow(dscolor.rgb, vec3(2.2));
	
	gPosition = mixPosition;
	gNormal = mixNormal;
	gColor = mixColor;
	
	outDsPosition = dsposition;
	outDsNormal = dsnormal;
	outDsColor = dscolor;
}