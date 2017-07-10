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

const float radius = 10;

out vec2 TexCoord;

void main()
{
	vec4 position = gl_in[0].gl_Position;
	
	gl_Position = projection * (position + vec4(-radius, -radius, 0.0, 0.0));
	TexCoord = vec2(0.0, 0.0);
    EmitVertex();

	gl_Position = projection * (position + vec4(radius, -radius, 0.0, 0.0));
	TexCoord = vec2(1.0, 0.0);
    EmitVertex();
	
	gl_Position = projection * (position + vec4(-radius, radius, 0.0, 0.0));
	TexCoord = vec2(0.0, 1.0);
    EmitVertex();
	
	gl_Position = projection * (position + vec4(radius, radius, 0.0, 0.0));
	TexCoord = vec2(1.0, 1.0);
    EmitVertex();
    
    EndPrimitive();
}  