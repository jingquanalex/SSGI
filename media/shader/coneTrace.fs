#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 outColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D inLight;
uniform sampler2D inReflection;
uniform sampler2D inReflectionRay;
uniform sampler2D inAmbientOcclusion;

uniform float roughness = 1;
uniform float sharpness = 0.2;
uniform float sharpnessPower = 2;
uniform float mipLevel = 0;
uniform float maxMipLevel = 5;
uniform vec2 bufferSize = vec2(1920, 1080);

// b = base, h = height, of isosceles triangle
float inRadius(float b, float h)
{
	return (b * (sqrt(b * b + 4 * h * h) - b)) / (4 * h);
}

void main()
{
	//vec3 position = texture(gPosition, TexCoord).xyz;
    //vec3 normal = texture(gNormal, TexCoord).xyz;
	vec4 light = texture(inLight, TexCoord);
	//vec4 reflection = textureLod(inReflection, TexCoord, mipLevel);
	vec4 reflectionRay = texture(inReflectionRay, TexCoord);
	//vec4 ao = textureLod(inAmbientOcclusion, TexCoord, mipLevel);
	
	float mRoughness = roughness;
	/*if (light.a == 0)
	{
		mRoughness = 0.55;
	}*/
	
	float distScaled = smoothstep(0.0, sharpness * pow(1 - mRoughness, sharpnessPower), reflectionRay.z);
	float mip = distScaled * maxMipLevel * mRoughness;
	//mip = 2;
	mip = clamp(mip, 0, maxMipLevel);
	float mipAlpha = clamp(mip * 1, 0, maxMipLevel);
	vec3 distColor = textureLod(inReflection, TexCoord, mip).rgb;
	float distAlpha = textureLod(inReflection, TexCoord, mipAlpha).a;
	vec4 finalColor = vec4(distColor, distAlpha);
	
	
	outColor = finalColor;
	//outColor = vec4(finalColor, light.a);
}