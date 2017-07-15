#version 450

in vec2 TexCoord;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gColor;

uniform sampler2D gInPosition;
uniform sampler2D gInNormal;
uniform sampler2D gInColor;
uniform sampler2D dsColor;
uniform sampler2D dsPosition;
uniform sampler2D dsNormal;

const float imgWidth = 640;
const float imgHeight = 480;

void main()
{
	vec3 position = texture(gInPosition, TexCoord).xyz;
    vec3 normal = texture(gInNormal, TexCoord).xyz;
    vec4 color = texture(gInColor, TexCoord);
	
	// Depth sensor outputs
	vec4 dscolor = texture(dsColor, TexCoord);
	vec3 dsposition = texture(dsPosition, TexCoord).rgb;
	vec3 dsnormal = texture(dsNormal, TexCoord).rgb;
	
	// Mix gBuffer and kinect position and color
	vec3 mixPosition = dsposition;
	vec4 mixColor = dscolor;
	vec3 mixNormal = dsnormal;
	
	if (dsposition.z < position.z)
	{
		mixPosition = position;
		mixColor.rgb = color.rgb;
		mixNormal = normal;
	}
	
	if (color.a == 0)
	{
		mixPosition = dsposition;
		mixColor.rgb = dscolor.rgb;
		mixNormal = dsnormal;
	}
	
	color = mixColor;
	position = mixPosition;
	normal = mixNormal;
	
	gPosition = position;
	gNormal = normal;
	gColor = color;
}