#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 outColor;

uniform sampler2D inColor;

uniform int isVertical = 0;
uniform int kernelRadius = 3;
uniform float kernel[128];
uniform int mipLevel = 0;

void main()
{
	int miplevel = mipLevel;
	vec4 color = textureLod(inColor, TexCoord, miplevel);
	
	if (miplevel == 0)
	{
		outColor = color;
		return;
	}
	
	vec2 texelSize = 1 / vec2(textureSize(inColor, 0));
	
	if (isVertical == 1)
	{
		texelSize = vec2(0, texelSize.y);
	}
	else
	{
		texelSize = vec2(texelSize.x, 0);
		miplevel = miplevel - 1;
	}
	
	// Gaussian filter
	vec4 accumValue = vec4(0);
	float accumWeight = 0;
	
	for (int i = -kernelRadius; i <= kernelRadius; i++)
	{
		vec2 sampleCoord = TexCoord + i * texelSize;
		vec4 sampleValue = textureLod(inColor, sampleCoord, miplevel);
		float sampleWeight = kernel[kernelRadius + i];
		
		accumValue += sampleValue * sampleWeight;
		accumWeight += sampleWeight;
	}
	
	vec4 finalColor = accumValue / accumWeight;
	
	outColor = finalColor;
}