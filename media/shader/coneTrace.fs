#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 outColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D inLight;
uniform sampler2D inReflection;
uniform sampler2D inReflectionRay;
uniform sampler2D inAmbientOcclusion;

uniform float sharpness = 0.2;
uniform float sharpnessPower = 1;
uniform float mipLevel = 0;
uniform float maxMipLevel = 5;

void main()
{
	float roughness = texture(gPosition, TexCoord).a;
    //float metallic = texture(gNormal, TexCoord).a;
	vec4 light = texture(inLight, TexCoord);
	//vec4 reflection = textureLod(inReflection, TexCoord, mipLevel);
	vec4 reflectionRay = texture(inReflectionRay, TexCoord);
	//vec4 ao = textureLod(inAmbientOcclusion, TexCoord, mipLevel);
	
	float mRoughness = roughness;
	//mRoughness = texture(gPosition, reflectionRay.xy).a; // sample rough parm at hitcoord
	
	//float distScaled = smoothstep(0.0, sharpness * pow(1 - mRoughness, sharpnessPower), reflectionRay.z);
	float distScaled = smoothstep(0.0, sharpness, pow(reflectionRay.z, sharpnessPower));
	float mip = distScaled * maxMipLevel * mRoughness;
	//float mip = maxMipLevel * mRoughness;
	//mip = 2;
	mip = clamp(mip, 0, maxMipLevel);
	vec3 distColor = textureLod(inReflection, TexCoord, mip).rgb;
	
	float mipAlpha = clamp(mip * 1, 0, maxMipLevel);
	float distAlpha = textureLod(inReflection, TexCoord, mipAlpha).a;
	vec4 finalColor = vec4(distColor, distAlpha);
	
	
	outColor = finalColor;
	//outColor = vec4(finalColor, light.a);
}