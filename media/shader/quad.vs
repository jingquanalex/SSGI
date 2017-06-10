#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texcoord;

layout (std140, binding = 0) uniform MatCam
{
    mat4 projection;
    mat4 view;
};

out vec2 Texcoord;

void main()
{
	Texcoord = texcoord;
	gl_Position = projection * view * vec4(position, 1.0);
    //gl_Position = vec4(position.xy, 0.0, 1.0);
}