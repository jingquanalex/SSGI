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
	float normalizedX = texcoord.x - 0.5;
	float normalizedY = texcoord.y - 0.5;
	float z = texture(samplerDepth, texcoord).r;
	float x = normalizedX * z * fx;
	float y = normalizedY * z * fy;
	z = z - 1;
	return vec3(x, y, z);
}

void main()
{
	vec3 position = texture(gInPosition, TexCoord).xyz;
    vec3 normal = texture(gInNormal, TexCoord).xyz;
    vec4 color = texture(gInColor, TexCoord);
	
	// Depth sensor textures
	vec4 dscolor = texture(dsColor, TexCoord);
	float dsdepth = texture(dsDepth, TexCoord).r;
	
	// Reconstructed position from depth (kinect)
	vec3 dsposition = dsDepthToWorldPosition(dsDepth, TexCoord);
	
	
	vec3 mixPosition = position;
	vec4 mixColor = color;
	//mixColor.a = color.a;
	
	if (position.z < dsposition.z)
	{
		mixPosition = dsposition;
		mixColor.rgb = dscolor.rgb;
	}
	
	if (color.a == 0)
	{
		mixPosition = dsposition;
		mixColor.rgb = dscolor.rgb;
	}
	
	color = mixColor;
	position = mixPosition;
	
	gPosition = position;
	//gPosition = vec3(-(position.z));
	gNormal = normal;
	gColor = color;
}