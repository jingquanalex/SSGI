#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texcoord;

out vec2 TexCoord;
out vec3 RayDirection;
out mat4 ProjectionTexture;

layout (std140, binding = 9) uniform MatCam
{
    mat4 projection;
	mat4 projectionInverse;
    mat4 view;
	mat4 viewInverse;
	mat4 kinectProjection;
	mat4 kinectProjectionInverse;
};

const mat4 mTexture = mat4
(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0
);

void main()
{
    gl_Position = vec4(position.xy, 0.0, 1.0);
	TexCoord = texcoord;
	
	vec4 cameraRay = vec4(texcoord.xy * 2.0 - 1.0, 0.0, 1.0);
	cameraRay = kinectProjectionInverse * cameraRay;
	RayDirection = cameraRay.xyz / cameraRay.w;
	ProjectionTexture = mTexture * kinectProjection;
}