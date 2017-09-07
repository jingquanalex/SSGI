#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 outColor;
//layout (location = 1) out vec4 outReflection;

layout (std140, binding = 9) uniform MatCam
{
    mat4 projection;
	mat4 projectionInverse;
    mat4 view;
	mat4 viewInverse;
	mat4 kinectProjection;
	mat4 kinectProjectionInverse;
};

//uniform vec2 bufferSize = vec2(1920, 1080);
//uniform vec2 texelSize = 1.0 / vec2(1920, 1080);

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColor;
uniform sampler2D aoMap;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;
uniform sampler2D reflectionMap;
uniform sampler2D reflectanceMap;

// PBR variables
uniform vec3 cameraPosition;
uniform vec3 lightPosition;
uniform vec3 lightColor;

const float pi = 3.1415926;

// Helper functions

vec3 depthToViewPosition(float depth, vec2 texcoord)
{
    vec4 clipPosition = vec4(texcoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPosition = projectionInverse * clipPosition;
    return viewPosition.xyz / viewPosition.w;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    //return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
	return F0 + (1.0 - F0) * pow(2.0, (-5.55473 * cosTheta - 6.98316) * cosTheta);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}   

vec3 render(vec3 P, vec3 N, vec3 csN, vec4 inColor, vec4 inReflection, vec3 ao, float mRoughness, float mMetallic, out vec3 outEnvColor)
{
	vec3 albedo = inColor.rgb;
	
	vec3 V = normalize(cameraPosition - P);
	//vec3 V = normalize(-P);
	vec3 R = reflect(-V, N); 
	
	// calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 F0 = mix(vec3(0.04), albedo, mMetallic);
	
	// ambient lighting (we now use IBL as the ambient term)
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, mRoughness);
    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - mMetallic;
	
	vec3 irradiance = texture(irradianceMap, N).rgb;
	//vec2 sampleCoord = vec2(1080 / 1920.0, 1) * csN.xy;
	//sampleCoord = sampleCoord * 0.5 + 0.5;
	//irradiance = texture(reflectanceMap, sampleCoord).rgb;
	vec3 diffuse = irradiance * albedo;
	
	// sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 5.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R, mRoughness * MAX_REFLECTION_LOD).rgb;
	vec4 reflectionColor = inReflection;
	//reflectionColor.rgb += prefilteredColor;
	//reflectionColor.rgb = mix(reflectionColor.rgb, reflectionColor.rgb + prefilteredColor, 1 - pow(1 - metallic, 5));
	//reflectionColor.rgb = mix(reflectionColor.rgb, prefilteredColor, roughness);
	//reflectionColor.rgb = mix(reflectionColor.rgb, reflectionColor.rgb * irradiance, 1 - pow(1 - metallic, 5));
	//reflectionColor.a *= 1 - pow(metallic, 8);
	vec3 envColor = mix(prefilteredColor, reflectionColor.rgb, reflectionColor.a);
	//envColor = prefilteredColor;
	//envColor = reflectionColor.rgb;
	
	//vec3 dscolor = texture(dsColor, TexCoord).rgb;
	//envColor = mix(envColor, dscolor, 1-inColor.a);
	outEnvColor = envColor;
	
    vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), mRoughness)).rg;
    vec3 specular = envColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;
    vec3 color = ambient;
	
	
	return color;
}


// Main

void main()
{
	//float depth = texture(gDepth, TexCoord).r;
	vec4 position = texture(gPosition, TexCoord);
	vec3 positionWorld = (viewInverse * vec4(position.xyz, 1)).xyz;
    vec4 normal = texture(gNormal, TexCoord);
	vec3 normalWorld = mat3(viewInverse) * normal.xyz;
    vec4 color = texture(gColor, TexCoord);
	vec4 reflection = texture(reflectionMap, TexCoord);
	float roughness = position.a;
	float metallic = normal.a;
	
	// Reconstruct position from depth buffer
	//position = depthToViewPosition(depth, TexCoord);
	
	
	vec4 finalColor = vec4(0);
	vec3 ao = texture(aoMap, TexCoord).rgb;
	vec3 envColor;
	finalColor.rgb = render(positionWorld, normalWorld, normal.xyz, color, reflection, ao, roughness, metallic, envColor);
	
	/*vec2 sampleCoord = vec2(1080 / 1920.0, 1) * normal.xy;
	sampleCoord = sampleCoord * 0.5 + 0.5;
	vec3 reflColor = texture(reflectanceMap, sampleCoord).rgb;
	finalColor.rgb /= reflColor;*/

	outColor = finalColor;
	outColor.a = color.a;
	
	//outReflection = reflection;
}