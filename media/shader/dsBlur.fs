#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 dsOutColor;
layout (location = 1) out float dsOutDepth;

uniform sampler2D dsColor;
uniform sampler2D dsDepth;

uniform int isVertical = 0;
uniform int kernelRadius = 4;
uniform float kernel[128];

void main()
{
	// Depth sensor textures
	vec4 dscolor = texture(dsColor, TexCoord);
	float dsdepth = texture(dsDepth, TexCoord).r;
	vec2 texelSize = 1 / vec2(textureSize(dsDepth, 0));
	
	if (isVertical == 1) texelSize = vec2(0, texelSize.y);
	else texelSize = vec2(texelSize.x, 0);
	
	// Gaussian filter
	float accumValue = 0;
	float accumWeight = 0;
	
	for (int i = -kernelRadius; i <= kernelRadius; i++)
	{
		vec2 sampleCoord = TexCoord + i * texelSize;
		float sampleValue = texture(dsDepth, sampleCoord).r;
		float sampleWeight = kernel[kernelRadius + i];
		
		accumValue += sampleValue * sampleWeight;
		accumWeight += sampleWeight;
	}
	
	float finalValue = accumValue / accumWeight;
	
	dsdepth = finalValue;
	
	dsOutColor = dscolor;
	dsOutDepth = dsdepth;
}