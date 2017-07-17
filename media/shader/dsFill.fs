#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 dsOutColor;
layout (location = 1) out float dsOutDepth;

uniform sampler2D dsColor;
uniform sampler2D dsDepth;

vec3 RGBToXYZ(vec3 inColor)
{
	mat3 MatToXYZ = mat3(
		0.4124564, 0.3575761, 0.1804375,
		0.2126729, 0.7151522, 0.0721750,
		0.0193339, 0.1191920, 0.9503041);
	return MatToXYZ * inColor;
}

void main()
{
	// Depth sensor textures
	vec4 dscolor = texture(dsColor, TexCoord);
	float dsdepth = textureLod(dsDepth, TexCoord, 0).r;
	
	vec2 texelSize = 1.0 / textureSize(dsDepth, 0).xy;
	
	
	// Fill hole by picking nearest neighbor with similar color
	/*if (dsdepth == 0)
	{
		int kernelRadius = 5;
		float bestDepth = 0, minColorDiff = 99999, minDist = 99999;
		for (int i = -kernelRadius; i <= kernelRadius; i++)
		{
			for (int j = -kernelRadius; j <= kernelRadius; j++)
			{
				vec2 offset = vec2(i, j) * texelSize;
				float offsetDepth = texture(dsDepth, TexCoord + offset).r;
				vec3 offsetColor = texture(dsColor, TexCoord + offset).rgb;
				
				vec3 colorDiffv = RGBToXYZ(offsetColor) - RGBToXYZ(dscolor.rgb);
				float colorDiff = dot(colorDiffv, colorDiffv);
				float dist = dot(vec2(i, j), vec2(i, j));
				if (offsetDepth > 0 && colorDiff < minColorDiff)
				//if (offsetDepth > 0 && dist < minDist)
				{
					minDist = dist;
					minColorDiff = colorDiff;
					bestDepth = offsetDepth;
				}
			}
		}
		
		dsdepth = bestDepth;
	}*/
	
	if (dsdepth == 0)
	{
		int kernelRadius = 35;
		float bestDepth = 0, minColorDiff = 99999, minDist = 99999;
		for (int i = -kernelRadius; i <= kernelRadius; i++)
		{
			for (int j = -kernelRadius; j <= kernelRadius; j++)
			{
				vec2 offset = vec2(i, j) * texelSize;
				float offsetDepth = texture(dsDepth, TexCoord + offset).r;
				vec3 offsetColor = texture(dsColor, TexCoord + offset).rgb;
				
				vec3 colorDiffv = RGBToXYZ(offsetColor) - RGBToXYZ(dscolor.rgb);
				float colorDiff = dot(colorDiffv, colorDiffv);
				float dist = dot(vec2(i, j), vec2(i, j));
				//if (offsetDepth > 0 && colorDiff < minColorDiff)
				if (offsetDepth > 0 && dist < minDist)
				{
					minDist = dist;
					minColorDiff = colorDiff;
					bestDepth = offsetDepth;
				}
			}
		}
		
		//dsdepth = bestDepth;
	}
	
	dsOutColor = dscolor;
	dsOutDepth = dsdepth;
}