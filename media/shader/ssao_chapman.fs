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


void main()
{
	vec3 position = texture(gPosition, TexCoord).xyz;
    vec3 normal = texture(gNormal, TexCoord).xyz;
	
	float occlusion = 0.0;
	vec3 sumColor = vec3(0);
	
	vec3 randomVec = vec3(1, 0, 0);
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);
	
	for(int i = 0; i < kernelSize; ++i)
	{
		// transform offset vectors from tangent space to view space
		vec3 offsetVec = TBN * inSamples[i];
		vec3 fragPos = position + offsetVec * kernelRadius;
		
		// transform offset vectors from view space to window space
		vec4 offset = kinectProjection * vec4(fragPos, 1.0);
		offset.xy /= offset.w;
		offset.xy = offset.xy * 0.5 + 0.5;
		
		float sampleDepth = texture(gPosition, offset.xy).z;
		
		if (sampleDepth > fragPos.z + bias)
		{
			vec3 sampleColor = texture(gColor, offset.xy).rgb;
			float range = smoothstep(kernelRadius, kernelRadius * 123, kernelRadius / abs(position.z - sampleDepth));
			
			occlusion += range;
			sumColor += (1 - sampleColor) * range;
		}
	}
	occlusion = occlusion / kernelSize;
	sumColor = sumColor / kernelSize;
	
	occlusion = clamp(1 - occlusion * intensity, 0, 1);
	occlusion = pow(occlusion, power);
	
	float colorIntensity = intensity * 1;
	float colorPower = power * 1;
	sumColor = clamp(1 - sumColor * colorIntensity, 0, 1);
	sumColor = vec3(pow(sumColor.r, colorPower), pow(sumColor.g, colorPower), pow(sumColor.b, colorPower));
	
	outColor = vec3(1 * occlusion);
}