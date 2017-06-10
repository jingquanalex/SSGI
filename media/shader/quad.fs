#version 450

in vec2 Texcoord;

out vec4 outColor;

uniform sampler2D colormap;

void main()
{
	outColor = vec4(0.9, 0.7, 0.2, 1.0);
	//outColor = texture(colormap, Texcoord);
}