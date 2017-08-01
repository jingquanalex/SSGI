#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 outColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D inRay;
uniform sampler2D inLight;
uniform sampler2D inColorTest;

uniform float roughness = 0.24;
uniform float mipLevels = 11;
uniform vec2 bufferSize = vec2(1920, 1080);


void main()
{
	vec3 position = texture(gPosition, TexCoord).xyz;
    vec3 normal = texture(gNormal, TexCoord).xyz;
	vec4 ray = texture(inRay, TexCoord);
	vec4 light = texture(inLight, TexCoord);
	vec4 colorTest = texture(inColorTest, TexCoord);
	
	
	float mipLevel = 0;
	vec3 finalColor = vec3(0);
	vec2 hitCoord = ray.xy;
	
	
	vec3 reflectedColor = textureLod(inLight, hitCoord, mipLevel).rgb;
	finalColor = mix(light.rgb, reflectedColor, ray.w);
	
	//finalColor = reflectedColor * ray.w;
	finalColor = light.rgb;
	//finalColor = vec3(colorTest.a);
	
	
	outColor = vec4(finalColor, light.a);
}