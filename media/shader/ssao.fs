#version 450

in vec2 TexCoord;

layout (location = 0) out float outColor;

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

// SSAO variables
const int kernelSize = 64;
uniform float kernelRadius = 0.35;
uniform float sampleBias = 0.005;
uniform float intensity = 1.0;
uniform float power = 1.0;
uniform sampler2D inNoise;
uniform vec3 inSamples[kernelSize];


void main()
{
	vec3 position = texture(gPosition, TexCoord).xyz;
    vec3 normal = texture(gNormal, TexCoord).xyz;
	
	// SSAO occlusion
	float occlusion = 0.0;
	
	//vec3 randomVec = texture(inNoise, TexCoord).xyz;
	vec3 randomVec = vec3(1, 0, 0);
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);
	
	for(int i = 0; i < kernelSize; ++i)
	{
		vec3 fsample = TBN * inSamples[i];
		fsample = position + fsample * kernelRadius;
		
		// view space offset vectors to window space
		vec4 offset = kinectProjection * vec4(fsample, 1.0);
		offset.xyz /= offset.w;
		offset.xyz = offset.xyz * 0.5 + 0.5;
		
		float sampleDepth = texture(gPosition, offset.xy).z;
		float rangeCheck = smoothstep(0.0, 1.0, kernelRadius / abs(position.z - sampleDepth));
		occlusion += (sampleDepth >= fsample.z + sampleBias ? 1.0 : 0.0) * rangeCheck;
	}
	occlusion = occlusion / kernelSize;
	
	occlusion = clamp(1 - occlusion * intensity, 0, 1);
	occlusion = pow(occlusion, power);
	
	
	outColor = occlusion;
}