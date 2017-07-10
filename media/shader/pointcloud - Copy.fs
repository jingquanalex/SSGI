#version 450

layout (std140, binding = 9) uniform MatCam
{
    mat4 projection;
	mat4 projectionInverse;
    mat4 view;
	mat4 viewInverse;
	mat4 kinectProjection;
	mat4 kinectProjectionInverse;
};

in vec2 TexCoord;

out vec4 outColor;

void main()
{
	/*vec3 N;
	N.xy = TexCoord * 2.0 - 1.0;
	float r2 = dot(N.xy, N.xy);
	if (r2 > 1.0) discard;
	N.z = sqrt(1.0 - r2);*/
	
	// Eye space depth
	/*float z = eyepos.z + N.z * radius;
	z = far * (z + near) / (z * (far - near));
	gl_FragDepth = z;*/
	
	outColor = vec4(1, 0, 0, 1);
}