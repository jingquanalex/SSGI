#version 450

layout (location = 1) in vec2 texcoord;

layout (std140, binding = 9) uniform MatCam
{
    mat4 projection;
	mat4 projectionInverse;
    mat4 view;
	mat4 viewInverse;
	mat4 kinectProjection;
	mat4 kinectProjectionInverse;
};

void main()
{
	gl_Position = view * vec4(0, 0, 0, 1.0);
}