#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texcoord;
layout (location = 2) in vec3 normal;

out vec3 Position;
out vec2 TexCoord;
out vec3 Normal;

layout (std140, binding = 9) uniform MatCam
{
    mat4 projection;
	mat4 projectionInverse;
    mat4 view;
	mat4 viewInverse;
	mat4 kinectProjection;
	mat4 kinectProjectionInverse;
};

uniform mat4 model;
uniform mat4 modelInverse;

void main()
{
	vec4 viewPos = view * model * vec4(position, 1.0);
	Position = viewPos.xyz;
	Normal = mat3(transpose(inverse(view * model))) * normal;
	//Normal = mat3(transpose(modelInverse)) * normal;
	TexCoord = texcoord;
	gl_Position = kinectProjection * viewPos;
}