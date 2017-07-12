#version 450

layout (location = 0) in ivec2 texelCoord;

layout (std140, binding = 9) uniform MatCam
{
    mat4 projection;
	mat4 projectionInverse;
    mat4 view;
	mat4 viewInverse;
	mat4 kinectProjection;
	mat4 kinectProjectionInverse;
};

uniform sampler2D dsColor;
uniform sampler2D dsDepth;

out vec4 gsPosition;
out vec4 gsPositionWorld;
out vec4 gsColor;

const float imgWidth = 640;
const float imgHeight = 480;
const float fx = tan(radians(61.9999962) / 2) * 2;
const float fy = tan(radians(48.5999985) / 2) * 2;

vec3 dsDepthToWorldPosition(sampler2D samplerDepth, ivec2 texcoord)
{
	float normalizedX = texcoord.x / imgWidth - 0.5;
	float normalizedY = 0.5 - texcoord.y / imgHeight;
	float z = texelFetch(samplerDepth, texcoord, 0).r;
	float x = normalizedX * z * fx;
	float y = normalizedY * z * fy;
	return vec3(x, y, z);
}

void main()
{
	ivec2 filpTexelCoord = ivec2(texelCoord.x, imgHeight - texelCoord.y);
	//gsPositionWorld = vec4(texelCoord, texelFetch(dsDepth, filpTexelCoord, 0).r, 1);
	vec3 dsPos = dsDepthToWorldPosition(dsDepth, filpTexelCoord);
	gsPositionWorld = vec4(dsPos, 1);
	gsPosition = view * gsPositionWorld;
	gsColor = texelFetch(dsColor, filpTexelCoord, 0);
}