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

const ivec2 bufferSize = ivec2(1920, 1080);
const vec2 texelSize = 1.0 / vec2(1920, 1080);

void main()
{
	ivec2 pixelCoord = ivec2(TexCoord * bufferSize);
	vec3 normal = texelFetch(gNormal, pixelCoord, 0).xyz;
	vec4 color = texelFetch(inColor, pixelCoord, 0);
	
	
	// For each pixel see if the normal vector in screenspace falls on current coordinate, if so, sum.
	vec3 colorSum = color.rgb;
	float colorWeight = 1;
	/*for (int i = 0; i < bufferSize.x; i++)
	{
		for (int j = 0; j < bufferSize.y; j++)
		{
			vec3 sampleColor = texelFetch(inColor, ivec2(i, j), 0).xyz;
			vec3 sampleNormal = texelFetch(gNormal, ivec2(i, j), 0).xyz;
			
			vec4 ssCoord = kinectProjection * vec4(sampleNormal, 1);
			ssCoord = ssCoord / ssCoord.w;
			ssCoord.xy = ssCoord.xy * 0.5 + 0.5;
			
			ivec2 samplePixelCoord = ivec2(ssCoord.xy * bufferSize);
			
			vec2 coordDist = pixelCoord - samplePixelCoord;
			float s = float(all(not(bvec2(coordDist))));
			
			
			colorSum += sampleColor * s;
			colorWeight += s;
		}
	}*/
	colorSum /= colorWeight;
	
	vec4 finalColor = vec4(0);
	
	finalColor.xyz = colorSum;
	
	outColor = finalColor;
}