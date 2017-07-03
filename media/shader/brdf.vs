#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texcoord;

out vec2 TexCoord;

void main()
{
    TexCoord = texcoord;
	gl_Position = vec4(position, 1.0);
}