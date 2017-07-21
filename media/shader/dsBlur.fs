#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 dsOutColor;
layout (location = 1) out float dsOutDepth;

uniform sampler2D dsColor;
uniform sampler2D dsDepth;

uniform int kernelRadius = 4;
//uniform float kernel[64];

uniform float sigma = 1.0;
uniform float bsigma = 0.01;
uniform float bsigmaJBF = 0.00001;
uniform float sthresh = 0.02;

float normpdf(float x, float s)
{
	return 0.3989422 * exp(-x * x / (2 * s * s)) / s;
}

// ref: https://github.com/tobspr/GLSL-Color-Spaces/blob/master/ColorSpaces.inc.glsl
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
	float dsdepth = texture(dsDepth, TexCoord).r;
	vec2 texelSize = 1 / vec2(textureSize(dsDepth, 0));
	
	// Combined bilateral filter
	float kernel[64];
	for (int i = 0; i <= kernelRadius; i++)
	{
		kernel[kernelRadius + i] = kernel[kernelRadius - i] = normpdf(i, sigma);
	}
	
	float accumValueBF = 0, accumValueJBF = 0;
	float accumWeightBF = 0, accumWeightJBF = 0;
	vec3 accumColor = vec3(0);
	float normFactor = 1.0 / normpdf(0, bsigma);
	for (int i = -kernelRadius; i <= kernelRadius; i++)
	{
		for (int j = -kernelRadius; j <= kernelRadius; j++)
		{
			float sampleValueD = texture(dsDepth, TexCoord + vec2(i, j) * texelSize).r;
			float sampleWeightD = normpdf(dsdepth - sampleValueD, bsigma) * normFactor * kernel[kernelRadius + i] * kernel[kernelRadius + j];
			
			accumValueBF += sampleValueD * sampleWeightD;
			accumWeightBF += sampleWeightD;
			
			vec3 sampleValueC = texture(dsColor, TexCoord + vec2(i, j) * texelSize).rgb;
			vec3 colorDiff = RGBToXYZ(dscolor.rgb) - RGBToXYZ(sampleValueC);
			float sampleWeightC = normpdf(dot(colorDiff, colorDiff), bsigmaJBF) * normFactor * kernel[kernelRadius + i] * kernel[kernelRadius + j];
			
			accumValueJBF += sampleValueD * sampleWeightC;
			accumWeightJBF += sampleWeightC;
			
			accumColor += sampleValueC * sampleWeightC;
		}
	}
	
	float BFValue = accumValueBF / accumWeightBF;
	float JBFValue = accumValueJBF / accumWeightJBF;
	vec3 colorValue = accumColor / accumWeightJBF;
	float deltaP = abs(JBFValue - BFValue);
	
	float inner = 3.1415926 / (2 * sthresh) * deltaP;
	float CBFValue = (deltaP > sthresh) ? JBFValue : cos(inner) * cos(inner) * BFValue + sin(inner) * sin(inner) * JBFValue;
	
	
	//dscolor = vec4(colorValue, 1);
	dsdepth = BFValue;
	
	
	dsOutColor = dscolor;
	dsOutDepth = dsdepth;
}