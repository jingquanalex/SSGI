#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normals;
layout (location = 2) in vec2 texcoord;

layout (std140, binding = 0) uniform MatCam
{
    mat4 projection;
    mat4 view;
};

out vec3 Normal;
out vec2 Texcoord;

void main()
{
	gl_Position = projection * view * vec4(position, 1.0);
	Normal = normals;
	Texcoord = texcoord;
}