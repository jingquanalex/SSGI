#version 450

in vec2 TexCoord;

layout (location = 0) out vec3 outColor;

uniform sampler2D inColor;
uniform sampler2D inNormal;

uniform int isVertical = 0;
uniform float kernel[128];
uniform int kernelRadius = 3;
uniform float sigma = 10;
uniform float bsigma = 0.1;

float normpdf(float x, float s)
{
	return 0.3989422 * exp(-x * x / (2.0 * s * s)) / s;
}

void main()
{
	// Depth sensor textures
	vec3 color = texture(inColor, TexCoord).rgb;
	vec3 normal = texture(inNormal, TexCoord).rgb;
	vec2 texelSize = 1.0 / vec2(textureSize(inColor, 0));
	
	// Cross bilateral filter
	vec3 accumValue = vec3(0);
	float accumWeight = 0;
	float normFactor = 1.0 / normpdf(0, bsigma);
	
	for (int i = -kernelRadius; i <= kernelRadius; i++)
	{
		vec2 sampleCoord = TexCoord + vec2(i, 0) * texelSize;
		if (isVertical == 1) sampleCoord = TexCoord + vec2(0, i) * texelSize;
		
		vec3 sampleValue = texture(inColor, sampleCoord).rgb;
		vec3 sampleNormal = texture(inNormal, sampleCoord).rgb;
		vec3 distv = normal - sampleNormal;
		float sampleWeight = normpdf(dot(distv, distv), bsigma) * normFactor * kernel[kernelRadius + i];
		
		accumValue += sampleValue * sampleWeight;
		accumWeight += sampleWeight;
	}
	
	color = accumValue / accumWeight;
	
	outColor = color;
}