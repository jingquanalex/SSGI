#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 outColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D inLight;
uniform sampler2D inReflection;
uniform sampler2D inAmbientOcclusion;

uniform float roughness = 0.24;
uniform float mipLevel = 4;
uniform vec2 bufferSize = vec2(1920, 1080);


void main()
{
	vec3 position = texture(gPosition, TexCoord).xyz;
    vec3 normal = texture(gNormal, TexCoord).xyz;
	vec4 light = texture(inLight, TexCoord);
	vec4 reflection = textureLod(inReflection, TexCoord, mipLevel);
	vec4 ao = textureLod(inAmbientOcclusion, TexCoord, mipLevel);
	
	vec3 finalColor = vec3(0);
	
	finalColor = mix(light.rgb, reflection.rgb, reflection.a);
	//finalColor = reflection.rgb * reflection.a;
	//finalColor = light.rgb;
	//finalColor = ao.rgb;
	
	outColor = vec4(finalColor, light.a);
}