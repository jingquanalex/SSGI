#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

out vec3 Position;
out vec3 Normal;
out vec2 Texcoord;

layout (std140, binding = 9) uniform MatCam
{
    mat4 projection;
	mat4 projectionInverse;
    mat4 view;
	mat4 viewInverse;
};

uniform mat4 model;
uniform mat4 modelInverse;

void main()
{
	// View space position and normal
	vec4 viewPos = view * model * vec4(position, 1.0);
	Position = viewPos.xyz;
	Normal = transpose(mat3(viewInverse * modelInverse)) * normal;
	Texcoord = texcoord;
	gl_Position = projection * viewPos;
}