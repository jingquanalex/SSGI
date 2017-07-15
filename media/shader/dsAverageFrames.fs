#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 dsOutColor;
layout (location = 1) out float dsOutDepth;

uniform sampler2D dsColor;
uniform sampler2DArray dsDepth;

void main()
{
	// Depth sensor textures (y is flipped)
	vec2 dsTexCoord = vec2(TexCoord.x, 1.0 - TexCoord.y);
	vec4 dscolor = texture(dsColor, dsTexCoord);
	float dsdepth = texture(dsDepth, vec3(dsTexCoord, 0)).r;
	int dsDepthLayers = textureSize(dsDepth, 0).z;
	
	// Fill missing depth from prev frames
	if (dsdepth < 0.00001)
	{
		for (int i = 1; i < dsDepthLayers; i++)
		{
			float dsdepthPrev = texture(dsDepth, vec3(dsTexCoord, i)).r;
			if (dsdepthPrev > 0)
			{
				dsdepth = dsdepthPrev;
				break;
			}
		}
	}
	else
	{
		// Average across frames
		int validCount = 1;
		for (int i = 1; i < dsDepthLayers; i++)
		{
			float dsdepthPrev = texture(dsDepth, vec3(dsTexCoord, i)).r;
			if (dsdepthPrev > 0)
			{
				dsdepth += dsdepthPrev;
				validCount++;
			}
		}
		dsdepth /= validCount;
	}
	
	dsOutColor = dscolor;
	dsOutDepth = dsdepth;
}