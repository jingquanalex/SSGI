#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 outColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D inLight;
uniform sampler2D inReflection;
uniform sampler2D inReflectionRay;
uniform sampler2D inAmbientOcclusion;

uniform float roughness = 0.99;
uniform float mipLevel = 4;
uniform float maxMipLevel = 11;
uniform vec2 bufferSize = vec2(1920, 1080);

// b = base, h = height, of isosceles triangle
float inRadius(float b, float h)
{
	return (b * (sqrt(b * b + 4 * h * h) - b)) / (4 * h);
}

void main()
{
	vec3 position = texture(gPosition, TexCoord).xyz;
    vec3 normal = texture(gNormal, TexCoord).xyz;
	vec4 light = texture(inLight, TexCoord);
	vec4 reflection = textureLod(inReflection, TexCoord, mipLevel);
	vec4 reflectionRay = texture(inReflectionRay, TexCoord);
	vec4 ao = textureLod(inAmbientOcclusion, TexCoord, mipLevel);
	
	vec3 reflectedColor = texture(inLight, reflectionRay.xy).rgb;
	
	// Cone tracing
	float specPower = 1000;
	float coneAngle = cos(pow(0.244, 1 / (specPower + 1)));
	
	vec2 coneVec = reflectionRay.xy - TexCoord;
	float adj = reflectionRay.z;
	float opp = 0;
	float r = 0;
	float mip = 0;
	
	vec4 totalColor = vec4(0);
	float totalWeight = 0;
	
	float maxMip = maxMipLevel;
	for (float i = 0; i < maxMip; i++)
	{
		opp = 2 * tan(coneAngle) * adj;
		r = inRadius(opp, adj);
		
		//vec2 samplePoint = TexCoord + normalize(coneVec) * (adj - r);
		
		mip = log2(r * max(bufferSize.x, bufferSize.y));
		//mip = roughness * maxMip * r;
		
		vec4 sampleColor = textureLod(inReflection, reflectionRay.xy, mip);
		float weight = pow(1 - i / maxMip, 1);
		
        totalColor += sampleColor * weight;
		totalWeight += weight;
		
		//if (mip > maxMipLevel) break;
		
		adj = adj - r * 2;
	}
	
	totalColor /= totalWeight;
	
	vec3 finalColor = vec3(0);
	
	
	//finalColor = ao.rgb;
	//finalColor = reflection.rgb * reflection.a;
	finalColor = mix(light.rgb, reflection.rgb, reflection.a);
	//finalColor = light.rgb;
	//finalColor = mix(light.rgb, (light.rgb + totalColor.rgb) / 2, reflection.a);
	//finalColor = mix(light.rgb, totalColor.rgb, reflection.a);
	//finalColor = vec3(totalColor.a);
	
	outColor = vec4(finalColor, reflection.a);
	//outColor = vec4(finalColor, light.a);
}