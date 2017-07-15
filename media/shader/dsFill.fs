#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 dsOutColor;
layout (location = 1) out float dsOutDepth;

uniform sampler2D dsColor;
uniform sampler2D dsDepth;

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
	// Depth sensor textures
	vec4 dscolor = texture(dsColor, TexCoord);
	float dsdepth = texture(dsDepth, TexCoord).r;
	
	// Fill hole by picking nearest neighbor
	if (dsdepth < 0.00001)
	{
		int kernelRadius = 55;
		float bestDepth = 0;
		for (int i = 1; i < kernelRadius; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				vec2 offset = i * neighborOffsets[j];
				float offsetDepth = texture(dsDepth, TexCoord + offset).r;
				
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
	dsOutDepth = dsdepth;
}