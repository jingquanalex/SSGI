#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 dsOutColor;
layout (location = 1) out float dsOutDepth;

uniform sampler2D dsColor;
uniform sampler2D dsDepth;

uniform int kernelRadius = 10;

/*vec3 RGBToXYZ(vec3 inColor)
{
	mat3 MatToXYZ = mat3(
		0.4124564, 0.3575761, 0.1804375,
		0.2126729, 0.7151522, 0.0721750,
		0.0193339, 0.1191920, 0.9503041);
	return MatToXYZ * inColor;
}
*/

void main()
{
	// Depth sensor textures
	vec4 dscolor = texture(dsColor, TexCoord);
	float dsdepth = texture(dsDepth, TexCoord).r;
	
	vec2 texelSize = 1.0 / textureSize(dsDepth, 0).xy;
	
	
	// Median filter (fill holes only)
	if (dsdepth == 0)
	{
		// Find mean within 3D kernel
		float meanDepth = 0;
		float validDepth = 0;
		for (int i = -kernelRadius; i <= kernelRadius; i++)
		{
			for (int j = -kernelRadius; j <= kernelRadius; j++)
			{
				vec2 offset = TexCoord + vec2(i, j) * texelSize;
				float offsetDepth = texture(dsDepth, offset).r;
				
				/*vec3 offsetColor = texture(dsColor, offset).rgb;
				vec3 colorDiffv = RGBToXYZ(dscolor.rgb) - RGBToXYZ(offsetColor);
				float colorDiff = dot(colorDiffv, colorDiffv);*/
				
				if (offsetDepth > 0)
				{
					meanDepth += offsetDepth;
					validDepth += 1;
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
				vec2 offset = TexCoord + vec2(i, j) * texelSize;
				float offsetDepth = texture(dsDepth, offset).r;
				
				float diffValue = abs(offsetDepth - meanDepth);
				if (diffValue < bestDiff)
				{
					bestDiff = diffValue;
					bestDepth = offsetDepth;
				}
			}
		}
		
		dsdepth = bestDepth;
	}
	
	dsOutColor = dscolor;
	dsOutDepth = dsdepth;
}