#version 450

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

layout (std140, binding = 9) uniform MatCam
{
    mat4 projection;
	mat4 projectionInverse;
    mat4 view;
	mat4 viewInverse;
	mat4 kinectProjection;
	mat4 kinectProjectionInverse;
};

uniform float radius = 0.005;

in vec4 gsPosition[];
in vec4 gsPositionWorld[];
in vec4 gsColor[];

out vec4 Position;
out vec4 PositionWorld;
out vec2 TexCoord;
out vec4 Color;
out float Radius;

void main()
{
	Position = gsPosition[0];
	PositionWorld = gsPositionWorld[0];
	Color = gsColor[0];
	Radius = radius;
	
	gl_Position = projection * (Position + vec4(-radius, -radius, 0.0, 0.0));
	TexCoord = vec2(0.0, 0.0);
	EmitVertex();

	gl_Position = projection * (Position + vec4(radius, -radius, 0.0, 0.0));
	TexCoord = vec2(1.0, 0.0);
	EmitVertex();
	
	gl_Position = projection * (Position + vec4(-radius, radius, 0.0, 0.0));
	TexCoord = vec2(0.0, 1.0);
	EmitVertex();
	
	gl_Position = projection * (Position + vec4(radius, radius, 0.0, 0.0));
	TexCoord = vec2(1.0, 1.0);
	EmitVertex();
	
    EndPrimitive();
}  