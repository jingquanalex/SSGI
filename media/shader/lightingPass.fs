#version 450

in vec2 Texcoord;

out vec4 outColor;

layout (std140, binding = 9) uniform MatCam
{
    mat4 projection;
	mat4 projectionInverse;
    mat4 view;
	mat4 viewInverse;
};

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColor;
uniform sampler2D gDepth;
uniform sampler2D dsColor;
uniform sampler2D dsDepth;
uniform float screenWidth;
uniform float screenHeight;

// Display mode:
// 1 - Full composite
// 2 - Color only
// 3 - SSAO only
uniform int displayMode = 1;

// SSAO variables
const int kernelSize = 64;
uniform float kernelRadius = 0.35f;
uniform float sampleBias = 0.005;
uniform sampler2D texNoise;
uniform vec3 samples[kernelSize];

vec3 getViewPositionFromDepth(float depth, vec2 texcoord)
{
    vec4 clipPosition = vec4(texcoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPosition = projectionInverse * clipPosition;
    return (viewPosition / viewPosition.w).xyz;
}

void main()
{
	// Position and normal are view space
	vec3 position = texture(gPosition, Texcoord).xyz;
    vec3 normal = texture(gNormal, Texcoord).xyz;
    vec4 color = texture(gColor, Texcoord);
	float depth = texture(gDepth, Texcoord).r;
	
	// Depth sensor textures
	vec2 Texcoordfv = vec2(Texcoord.x, 1.0 - Texcoord.y);
	vec4 dscolor = texture(dsColor, Texcoordfv);
	float dsdepth = texture(dsDepth, Texcoordfv).r;
	
	// Reconstructed position from depth
	position = getViewPositionFromDepth(depth, Texcoord);
	//position = getViewPositionFromDepth(dsdepth, Texcoordfv);
	
	// SSAO occlusion
	vec2 noiseScale = vec2(screenWidth / 4, screenHeight / 4);
	vec3 randomVec = texture(texNoise, Texcoord * noiseScale).xyz;
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);
	
	// Transform samples from tangent to view space
	float occlusion = 0.0;
	for(int i = 0; i < kernelSize; ++i)
	{
		vec3 fsample = TBN * samples[i];
		//vec3 fsample = samples[i];
		fsample = position + fsample * kernelRadius;
		
		vec4 offset = vec4(fsample, 1.0);
        offset = projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
		
		//float sampleDepth = texture(gPosition, offset.xy).z;
		float sampleDepth = getViewPositionFromDepth(texture(gDepth, offset.xy).r, offset.xy).z;
		//offset.y = 1.0 - offset.y;
		//float sampleDepth = getViewPositionFromDepth(texture(dsDepth, offset.xy).r, offset.xy).z;
		float rangeCheck = smoothstep(0.0, 1.0, kernelRadius / abs(position.z - sampleDepth));
		occlusion += (sampleDepth >= fsample.z + sampleBias ? 1.0 : 0.0) * rangeCheck;
	}
	occlusion = 1.0 - (occlusion / kernelSize);
	
	switch (displayMode)
	{
		case 1:
			outColor = color * occlusion;
			break;
		
		case 2:
			outColor = vec4(position, 1);
			break;
			
		case 3:
			outColor = vec4(occlusion);
			break;
			
		case 4:
			outColor = dscolor;
			break;
			
		case 5:
			outColor = vec4(dsdepth);
			break;
			
		case 6:
			outColor = dscolor * occlusion;
			break;
	}
}