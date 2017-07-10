#version 450

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gColor;

layout (std140, binding = 9) uniform MatCam
{
    mat4 projection;
	mat4 projectionInverse;
    mat4 view;
	mat4 viewInverse;
	mat4 kinectProjection;
	mat4 kinectProjectionInverse;
};

in vec4 Position;
in vec4 PositionWorld;
in vec2 TexCoord;
in vec4 Color;
in float Radius;

void main()
{
	vec3 N;
	N.xy = TexCoord * 2.0 - 1.0;
	float r2 = dot(N.xy, N.xy);
	if (r2 > 1.0) discard;
	N.z = sqrt(1.0 - r2);
	
	// Eye space depth
	/*float z = eyepos.z + N.z * Radius;
	z = far * (z + near) / (z * (far - near));
	gl_FragDepth = z;*/
	
	gPosition = (view * vec4(PositionWorld.xyz + N * Radius, 1)).xyz;
	gNormal = N; //TODO: world space normals
	gColor = Color;
}