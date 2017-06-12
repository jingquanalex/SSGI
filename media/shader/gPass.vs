#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

out vec3 Position;
out vec3 Normal;
out vec2 Texcoord;

layout (std140, binding = 0) uniform MatCam
{
    mat4 projection;
    mat4 view;
};

uniform mat4 model;

void main()
{
	Position = (model * vec4(position, 1.0)).xyz;
	Normal = normal;
	Texcoord = texcoord;
	gl_Position = projection * view * vec4(Position, 1.0);
}