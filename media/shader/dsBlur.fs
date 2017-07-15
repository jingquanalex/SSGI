#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 dsOutColor;
layout (location = 1) out float dsOutDepth;
layout (location = 2) out vec3 dsOutPosition;
layout (location = 3) out vec3 dsOutNormal;

uniform sampler2D dsColor;
uniform sampler2D dsDepth;
uniform sampler2D dsPosition;
uniform sampler2D dsNormal;

uniform float sigma = 1.0;
uniform float bsigma = 20.9;
const int msize = 8;

float normpdf(in float x, in float sigma)
{
	return 0.39894*exp(-0.5*x*x/(sigma*sigma))/sigma;
}

float normpdf3(in vec3 v, in float sigma)
{
	return 0.39894*exp(-0.5*dot(v,v)/(sigma*sigma))/sigma;
}

void main()
{
	// Depth sensor textures
	vec4 dscolor = texture(dsColor, TexCoord);
	float dsdepth = texture(dsDepth, TexCoord).r;
	vec3 dsposition = texture(dsPosition, TexCoord).rgb;
	vec3 dsnormal = texture(dsNormal, TexCoord).rgb;
	
	// Bilateral filter
	// https://github.com/mattdesl/lwjgl-basics/wiki/ShaderLesson5
	const int kSize = (msize - 1) / 2;
	float kernel[msize];
	vec3 final_colour = vec3(0.0);
	
	//create the 1-D kernel
	float Z = 0.0;
	for (int j = 0; j <= kSize; ++j)
	{
		kernel[kSize+j] = kernel[kSize-j] = normpdf(float(j), sigma);
	}
	
	vec2 texelSize = 1.0 / vec2(textureSize(dsPosition, 0));
	vec3 cc;
	float factor;
	float bZ = 1.0/normpdf(0.0, bsigma);
	//read out the texels
	for (int i=-kSize; i <= kSize; ++i)
	{
		for (int j=-kSize; j <= kSize; ++j)
		{
			cc = texture(dsNormal, TexCoord + vec2(i, j) * texelSize).rgb;
			factor = normpdf3(cc-dsnormal, bsigma)*bZ*kernel[kSize+j]*kernel[kSize+i];
			Z += factor;
			final_colour += factor*cc;

		}
	}
	
	dsnormal = final_colour;
	
	/*vec2 texelSize = 1.0 / vec2(textureSize(dsPosition, 0));
	vec3 result = vec3(0);
	vec2 hlim = vec2(float(-uBlurSize) * 0.5 + 0.5);
	for (int i = 0; i < uBlurSize; ++i)
	{
		for (int j = 0; j < uBlurSize; ++j)
		{
			vec2 offset = (hlim + vec2(i, j)) * texelSize;
			result += texture(dsNormal, TexCoord + offset).rgb;
		}
	}
	dsnormal = result / (uBlurSize * uBlurSize);*/
	
	dsOutColor = dscolor;
	dsOutDepth = dsdepth;
	dsOutPosition = dsposition;
	dsOutNormal = dsnormal;
}