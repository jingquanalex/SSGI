#version 450

in vec2 TexCoord;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gColor;

uniform sampler2D gInPosition;
uniform sampler2D gInNormal;
uniform sampler2D gInColor;
uniform sampler2D dsColor;
uniform sampler2D dsDepth;

const float imgWidth = 640;
const float imgHeight = 480;
const float fx = tan(radians(61.9999962) / 2) * 2;
const float fy = tan(radians(48.5999985) / 2) * 2;

vec3 dsDepthToWorldPosition(sampler2D samplerDepth, vec2 texcoord)
{
	float normalizedX = 0.5 - texcoord.x;
	float normalizedY = 0.5 - texcoord.y;
	float z = texture(samplerDepth, texcoord).r - 1;
	float x = normalizedX * z * fx;
	float y = normalizedY * z * fy;
	
	return vec3(x, y, z);
}

void main()
{
	vec3 position = texture(gInPosition, TexCoord).xyz;
    vec3 normal = texture(gInNormal, TexCoord).xyz;
    vec4 color = texture(gInColor, TexCoord);
	
	// Depth sensor outputs
	vec4 dscolor = texture(dsColor, TexCoord);
	float dsdepth = texture(dsDepth, TexCoord).r;
	vec3 dsposition = dsDepthToWorldPosition(dsDepth, TexCoord);
	
	// Mix gBuffer and kinect position and color
	vec3 mixPosition = dsposition;
	vec4 mixColor = dscolor;
	
	if (dsposition.z < position.z)
	{
		mixPosition = position;
		mixColor.rgb = color.rgb;
	}
	
	if (color.a == 0)
	{
		mixPosition = dsposition;
		mixColor.rgb = dscolor.rgb;
	}
	
	color = mixColor;
	position = mixPosition;
	
	gPosition = position;
	//gPosition = vec3(-position.z);
	gNormal = normal;
	gColor = color;
}