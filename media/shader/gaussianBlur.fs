#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outColor2;

uniform sampler2D inColor;
uniform sampler2D inColor2;
uniform sampler2D inPosition;

uniform int isVertical = 0;
uniform int mipLevel = 0;
uniform float kernel[128];
uniform int kernelRadius = 3;
uniform float bsigma = 0.1;

float normpdf(float x, float s)
{
	return 1 / (s * s * 2 * 3.14159265f) * exp(-x * x / (2.0 * s * s)) / s;
}

// Cross bilateral blur the light buffers
void main()
{
	int mip = mipLevel;
	
	if (mip == 0)
	{
		vec4 color = textureLod(inColor, TexCoord, mip);
		vec4 color2 = textureLod(inColor2, TexCoord, mip);
		outColor = color;
		outColor2 = color2;
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
		mip = mip - 1;
	}
	
	float Z = texture(inPosition, TexCoord).z;
	
	// Gaussian filter
	vec4 accumValue = vec4(0);
	vec4 accumValue2 = vec4(0);
	float accumWeight = 0;
	
	//kernelRadius = kernelRadius * mip;
	
	for (int i = -kernelRadius; i <= kernelRadius; i++)
	{
		vec2 sampleCoord = TexCoord + i * texelSize;
		vec4 sampleValue = textureLod(inColor, sampleCoord, mip);
		//float sampleZ = texture(inPosition, sampleCoord).z;
		//float sampleWeight = normpdf(Z - sampleZ, bsigma) * kernel[kernelRadius + i];
		float sampleWeight = kernel[kernelRadius + i];
		
		accumValue += sampleValue * sampleWeight;
		accumWeight += sampleWeight;
		
		vec4 sampleValue2 = textureLod(inColor2, sampleCoord, mip);
		accumValue2 += sampleValue2 * sampleWeight;
	}
	
	vec4 finalColor = accumValue / accumWeight;
	vec4 finalColor2 = accumValue2 / accumWeight;
	
	outColor = finalColor;
	//outColor2 = finalColor2;
}