#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 outColor;

layout (std140, binding = 9) uniform MatCam
{
    mat4 projection;
	mat4 projectionInverse;
    mat4 view;
	mat4 viewInverse;
	mat4 kinectProjection;
	mat4 kinectProjectionInverse;
};

uniform sampler2D gNormal;
uniform sampler2D inColor;
uniform samplerCube envMap;

const ivec2 bufferSize = ivec2(1920, 1080);
const vec2 texelSize = 1.0 / vec2(1920, 1080);
const float aspect = 1080.0 / 1920;


void main()
{
	vec3 normal = texture(gNormal, TexCoord).xyz;
	vec4 color = texture(inColor, TexCoord);
	
	vec4 envColor = texture(envMap, normal);
	
	// For each pixel see if the normal vector in screenspace falls on current coordinate, if so, sum.
	vec3 colorSum = vec3(0);
	float colorWeight = 1;
	for (int i = 0; i < bufferSize.x; i++)
	{
		for (int j = 0; j < bufferSize.y; j++)
		{
			vec2 sampleCoord = vec2(i, j) * texelSize;
			vec3 sampleColor = texture(inColor, sampleCoord).rgb;
			vec3 sampleNormal = texture(gNormal, sampleCoord).xyz;
			vec3 envColor = texture(envMap, sampleNormal).rgb;
			
			vec2 ssCoord = sampleNormal.xy;
			ssCoord.x *= aspect;
			ssCoord = ssCoord * 0.5 + 0.5;
			
			
			vec2 coordDist = abs(TexCoord - ssCoord);
			float s = float(all(not(bvec2(step(texelSize * 5, coordDist)))));
			
			
			colorSum += (sampleColor + envColor) * 0.5 * s;
			colorWeight += s;
		}
	}
	colorSum /= colorWeight;
	
	vec4 finalColor = vec4(0);
	
	finalColor.xyz = colorSum;
	
	outColor = finalColor;
}