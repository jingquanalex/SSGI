#version 450

in vec2 TexCoord;

out vec4 outColor;

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
uniform float kernelRadius = 0.35;
uniform float sampleBias = 0.005;
uniform sampler2D texNoise;
uniform vec3 samples[kernelSize];

float nearPlane = 0.1;
float farPlane = 100.0;

// Helper functions

vec3 depthToViewPosition(float depth, vec2 texcoord)
{
    vec4 clipPosition = vec4(texcoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPosition = projectionInverse * clipPosition;
    return (viewPosition / viewPosition.w).xyz;
}

vec3 kinectDepthToViewPosition(float depth, vec2 texcoord)
{
	float z = depth;
    vec4 clipPosition = vec4(texcoord * 2.0 - 1.0, z * 2.0 - 1.0, 1.0);
    vec4 viewPosition = kinectProjectionInverse * clipPosition;
    return (viewPosition / viewPosition.w).xyz;
}

const float fx = 1 / 532.5695; // 640 / (2*tan(deg2rad(61.9999962) / 2)) = 532.5695
const float fy = 1 / 531.5411; // 480 / (2*tan(deg2rad(48.5999985) / 2)) = 531.5411
const float cx = 640 / 2;
const float cy = 480 / 2;

vec3 kinectDepthToWorldPosition(float depth, vec2 texcoord)
{
	float z = depth * 10;
	float x = (texcoord.x * 640 - cx) * z * fx;
	float y = (texcoord.y * 480 - cy) * z * fy;
	return vec3(x, y, z);
}

vec4 circle(vec2 pos, float radius, vec3 color)
{
	vec2 vd = pos - TexCoord;
	vd.x *=  screenWidth / screenHeight;
	return vec4(color, step(length(vd), radius));
}

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane)) / farPlane;
}

// Main

void main()
{
	// Position and normal are view space
	vec3 position = texture(gPosition, TexCoord).xyz;
    vec3 normal = texture(gNormal, TexCoord).xyz;
    vec4 color = texture(gColor, TexCoord);
	float depth = texture(gDepth, TexCoord).r;
	
	// Depth sensor textures
	vec2 dsTexcoord = vec2(TexCoord.x, 1.0 - TexCoord.y);
	vec4 dscolor = texture(dsColor, dsTexcoord);
	float dsdepth = texture(dsDepth, dsTexcoord).r;
	
	// Reconstructed position from depth (kinect)
	//position = depthToViewPosition(depth, TexCoord);
	//position = kinectDepthToViewPosition(dsdepth, TexCoord);
	position = (viewInverse * vec4(position, 1)).xyz;
	
	// draw a disk at each kinect frag position
	vec4 circlePos = vec4(1, TexCoord.y*1000, 0, 1);
	circlePos = projection * view * circlePos;
	circlePos.xyz = circlePos.xyz / circlePos.w;
	circlePos.xy = (circlePos.xy + 1) * 0.5;
	
	if (circlePos.w > 0) // draw circle if in front of camera
	{
		vec4 layer2 = circle(circlePos.xy, 0.01, vec3(1,0,0));
		color = mix(color, layer2, layer2.a);
	}
	
	// SSAO occlusion
	vec2 noiseScale = vec2(screenWidth / 4, screenHeight / 4);
	vec3 randomVec = texture(texNoise, TexCoord * noiseScale).xyz;
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);
	
	// Transform samples from tangent to view space
	float occlusion = 0.0;
	for(int i = 0; i < kernelSize; ++i)
	{
		//vec3 fsample = TBN * samples[i];
		vec3 fsample = samples[i];
		fsample = position + fsample * kernelRadius;
		
		vec4 offset = vec4(fsample, 1.0);
        offset = projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
		
		//float sampleDepth = texture(gPosition, offset.xy).z;
		//float sampleDepth = depthToViewPosition(texture(gDepth, offset.xy).r, offset.xy).z;
		float sampleDepth = kinectDepthToWorldPosition(texture(dsDepth, offset.xy).r, offset.xy).z;
		float rangeCheck = smoothstep(0.0, 1.0, kernelRadius / abs(position.z - sampleDepth));
		occlusion += (sampleDepth >= fsample.z + sampleBias ? 1.0 : 0.0) * rangeCheck;
	}
	occlusion = 1.0 - (occlusion / kernelSize);
	
	switch (displayMode)
	{
		case 1:
			outColor = color;
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