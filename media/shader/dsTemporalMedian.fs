#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 dsOutColor;
layout (location = 1) out float dsOutDepth;

uniform sampler2D dsColor;
uniform sampler2DArray dsDepth;

uniform int kernelRadius = 1;
uniform int frameLayers;

void main()
{
	// Depth sensor textures (y is flipped)
	vec2 dsTexCoord = vec2(TexCoord.x, 1.0 - TexCoord.y);
	vec4 dscolor = texture(dsColor, dsTexCoord);
	float dsdepth = texture(dsDepth, vec3(dsTexCoord, 0)).r;
	
	vec2 texelSize = 1.0 / textureSize(dsDepth, 0).xy;
	//frameLayers = textureSize(dsDepth, 0).z;
	
	
	// Temporal median filter
	//if (dsdepth == 0)
	{
		// Find mean within 3D kernel
		float meanDepth = 0;
		float validDepth = 0;
		for (int i = -kernelRadius; i <= kernelRadius; i++)
		{
			for (int j = -kernelRadius; j <= kernelRadius; j++)
			{
				for (int k = 0; k < frameLayers; k++)
				{
					vec3 offset = vec3(dsTexCoord + vec2(i, j) * texelSize, k);
					float offsetDepth = texture(dsDepth, offset).r;
					
					if (offsetDepth > 0)
					{
						meanDepth += offsetDepth;
						validDepth += 1;
					}
				}
			}
		}
		meanDepth /= validDepth;
		
		// Find value closest to mean and set as result
		float bestDepth = 0, bestDiff = 1;
		for (int i = -kernelRadius; i <= kernelRadius; i++)
		{
			for (int j = -kernelRadius; j <= kernelRadius; j++)
			{
				for (int k = 0; k < frameLayers; k++)
				{
					vec3 offset = vec3(dsTexCoord + vec2(i, j) * texelSize, k);
					float offsetDepth = texture(dsDepth, offset).r;
					
					float diffValue = abs(offsetDepth - meanDepth);
					if (diffValue < bestDiff)
					{
						bestDiff = diffValue;
						bestDepth = offsetDepth;
					}
				}
			}
		}
		
		dsdepth = bestDepth;
	}
	
	
	dsOutColor = dscolor;
	dsOutDepth = dsdepth;
}