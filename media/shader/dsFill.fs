#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 dsOutColor;
layout (location = 1) out vec4 dsOutDepth;

uniform sampler2D dsColor;
uniform sampler2D dsDepth;
uniform sampler2D dsDepthPrev;

uniform float imgWidth = 640;
uniform float imgHeight = 480;

vec2 neighborOffsets[4] = vec2[]
(
	vec2(1 / imgWidth, 0),
    vec2(-1 / imgWidth, 0),
	vec2(0, 1 / imgHeight),
	vec2(0, -1 / imgHeight)
);

void main()
{
	// Depth sensor textures (y is flipped)
	vec2 dsTexCoord = vec2(TexCoord.x, 1.0 - TexCoord.y);
	vec4 dscolor = texture(dsColor, dsTexCoord);
	float dsdepth = texture(dsDepth, dsTexCoord).r;
	float dsdepthPrev = texture(dsDepthPrev, dsTexCoord).r;
	
	if (dsdepth < 0.00001)
	{
		// Fill missing depth from prev frame
		dsdepth = dsdepthPrev;
	}
	
	if (dsdepth < 0.00001)
	{
		// Fill hole by picking nearest neighbor
		int kernelRadius = 55;
		float bestDepth = 0;
		for (int i = 1; i < kernelRadius; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				vec2 offset = i * neighborOffsets[j];
				float offsetDepth = texture(dsDepth, dsTexCoord + offset).r;
				float offsetDepthPrev = texture(dsDepthPrev, dsTexCoord + offset).r;
				offsetDepth = max(offsetDepth, offsetDepthPrev);
				
				if (offsetDepth > 0.00001)
				{
					bestDepth = offsetDepth;
					break;
				}
			}
			
			if (bestDepth != 0) break;
		}
		dsdepth = bestDepth;
	}
	
	dsOutColor = dscolor;
	dsOutDepth = vec4(dsdepth);
}