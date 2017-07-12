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
	float normalizedY = 0.5 - texcoord.y;
	float z = texture(samplerDepth, texcoord).r;
	float x = normalizedX * z * fx;
	float y = normalizedY * z * fy;
	return vec3(x, y, z);
}


vec2 neighborOffsets[4] = vec2[]
(
	vec2(1 / imgWidth, 0),
    vec2(-1 / imgWidth, 0),
	vec2(0, 1 / imgHeight),
	vec2(0, -1 / imgHeight)
);

void main()
{
	vec3 position = texture(gInPosition, TexCoord).xyz;
    vec3 normal = texture(gInNormal, TexCoord).xyz;
    vec4 color = texture(gInColor, TexCoord);
	
	// Depth sensor textures
	vec2 dsTexCoord = vec2(TexCoord.x, 1.0 - TexCoord.y);
	vec4 dscolor = texture(dsColor, dsTexCoord);
	float dsdepth = texture(dsDepth, dsTexCoord).r;
	
	// Reconstructed position from depth (kinect)
	vec3 dsposition = dsDepthToWorldPosition(dsDepth, dsTexCoord);
	
	// Test: Fill hole in depth buffer - pick nearest adjacent neighbor
	if (dsdepth < 0.00001)
	{
		int kernelRadius = 25;
		vec3 nearestPos = vec3(0, 0, 0);
		for (int i = 1; i < kernelRadius; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				vec2 offset = i * neighborOffsets[j];
				float offsetDepth = texture(dsDepth, dsTexCoord + offset).r;
				
				if (offsetDepth > 0.001)
				{
					nearestPos = dsDepthToWorldPosition(dsDepth, dsTexCoord + offset);
					break;
				}
			}
			
			if (nearestPos != vec3(0, 0, 0)) break;
		}
		dsposition = nearestPos;
	}
	
	
	vec3 mixPosition = dsposition;
	vec4 mixColor = dscolor;
	mixColor.a = color.a;
	
	if (mixPosition.z < position.z)
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
	gNormal = normal;
	gColor = color;
}