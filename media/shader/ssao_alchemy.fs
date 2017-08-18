#version 450

in vec2 TexCoord;

layout (location = 0) out vec3 outColor;

layout (std140, binding = 9) uniform MatCam
{
    mat4 projection;
	mat4 projectionInverse;
    mat4 view;
	mat4 viewInverse;
	mat4 kinectProjection;
	mat4 kinectProjectionInverse;
};

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColor;

uniform vec3 inSamples[64];
uniform int kernelSize = 64;
uniform float kernelRadius = 0.35;
uniform float bias = 0.005;
uniform float intensity = 1.0;
uniform float power = 1.0;

const float epsilon = 0.0001;
uniform int samples = 9;
uniform vec2 bufferSize = vec2(1920, 1080);

// Alchemy AO random disc samples
vec2 randomSamples(int sampleNumber, int samples, ivec2 pxCoord)
{
	float radius = float(sampleNumber + 0.5) * (1.0 / samples);
	float hash = (3 * pxCoord.x ^ pxCoord.y + pxCoord.x * pxCoord.y) * 10.0;
	float angle = radius * (7 * 6.28) + hash;

	return radius * vec2(cos(angle), sin(angle));
}

void main()
{
	if (samples == 0)
	{
		outColor = vec3(1);
		return;
	}
	
	vec3 position = texture(gPosition, TexCoord).xyz;
    vec3 normal = texture(gNormal, TexCoord).xyz;
	
	float sumOcclusion = 0.0;
	vec3 sumColor = vec3(0);
	ivec2 ssC = ivec2(bufferSize * TexCoord);
	
	for (int i = 0; i < samples; i++)
	{
		vec2 unitOffset = randomSamples(i, samples, ssC);
		vec2 sampleCoord = TexCoord + unitOffset * kernelRadius;
		vec3 samplePosition = texture(gPosition, sampleCoord).xyz;
		//vec3 sampleColor = texture(gColor, sampleCoord).rgb;

		vec3 v = samplePosition - position;
		float vv = dot(v, v);
		float vn = dot(v, normal);
		float sD = max(pow(kernelRadius * kernelRadius - vv, 3), 0);
		float sS = max((vn - bias) / (vv + epsilon), 0);
		
		//sumColor += (1 - sampleColor) * sD * sS;
		sumOcclusion += sD * sS;
		//sumOcclusion += max((vn - bias) / (vv + epsilon), 0);
	}
	
	float occlusion = sumOcclusion * 5.0 / (pow(kernelRadius, 6) * samples);
	occlusion = max(1 - occlusion * intensity, 0);
	occlusion = pow(occlusion, power);
	
	/*float occlusion = max(1 - sumOcclusion * (intensity * 2) / samples, 0);
	occlusion = pow(occlusion, power);*/
	
	/*vec3 color = sumColor * 5.0 / (pow(kernelRadius, 6) * samples);
	color = max(1 - color * intensity * 1, 0);*/
	
	outColor = vec3(occlusion);
}