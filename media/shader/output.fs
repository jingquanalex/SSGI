#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 outColor;

uniform sampler2D inColor;

void main()
{
	outColor = texture(inColor, TexCoord);
}