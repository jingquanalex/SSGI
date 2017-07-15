#version 450

in vec2 TexCoord;

out vec4 outColor;

layout (std140, binding = 9) uniform MatCam
{
    mat4 projection;
	mat4 projectionInverse;
    mat4 view;
	mat4 viewInverse;
	mat4 kinectProjection;
	mat4 kinectProjectionInverse;
};

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColor;
uniform sampler2D gDepth;
uniform sampler2D dsColor;
uniform sampler2D dsDepth;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;
uniform float screenWidth;
uniform float screenHeight;

// Display mode:
// 1 - Full composite
// 2 - Color only
// 3 - SSAO only
uniform int displayMode = 1;

// SSAO variables
const int kernelSize = 64;
uniform float kernelRadius = 0.35;
uniform float sampleBias = 0.005;
uniform sampler2D texNoise;
uniform vec3 samples[kernelSize];

// PBR variables
uniform vec3 cameraPosition;
uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform float metallic;
uniform float roughness;

const float PI = 3.14159265359;

// Helper functions

vec3 depthToViewPosition(float depth, vec2 texcoord)
{
    vec4 clipPosition = vec4(texcoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPosition = projectionInverse * clipPosition;
    return (viewPosition / viewPosition.w).xyz;
}

const float fx = tan(radians(61.9999962) / 2) * 2;
const float fy = tan(radians(48.5999985) / 2) * 2;

vec3 dsDepthToWorldPosition(sampler2D samplerDepth, vec2 texcoord)
{
	float normalizedX = 0.5 - texcoord.x;
	float normalizedY = 0.5 - texcoord.y;
	float z = texture(samplerDepth, texcoord).r - 1;
	float x = normalizedX * z * fx;
	float y = normalizedY * z * fy;
	
	return vec3(x, y, z);
}

// Rendering equation, cook torrance brdf

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}   

vec3 render(vec3 P, vec3 N, vec4 inColor, float ao)
{
	vec3 albedo = pow(inColor.rgb, vec3(2.2));
	if (albedo == vec3(0)) albedo = vec3(1); // for no textures
	
	vec3 V = normalize(cameraPosition - P);
	//vec3 V = normalize(-P);
	vec3 R = reflect(-V, N); 
	
	// calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
	vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
	
	// reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 1; ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(lightPosition - P);
        vec3 H = normalize(V + L);
        float distance = length(lightPosition - P);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColor * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);    
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);        
        
        vec3 nominator    = NDF * G * F;
        float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // 0.001 to prevent divide by zero.
        vec3 specular = nominator / denominator;
        
         // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	                
            
        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }
	
	// ambient lighting (we now use IBL as the ambient term)
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
	
	vec3 irradiance = texture(irradianceMap, N).rgb;
	vec3 diffuse = irradiance * albedo;
	
	// sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 5.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;
    
    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2));
	
	// Mix shade on kinect outputs
	/*if (inColor.a < 1.0 || P.rgb == vec3(0.2))
	{
		//if (inColor.a == vec3(0)) ao = 1;
		color = vec3(ao);
	}*/
	
	return vec3(color);
}

// Main

void main()
{
	float depth = texture(gDepth, TexCoord).r;
	vec3 position = texture(gPosition, TexCoord).xyz;
	vec3 positionWorld = (viewInverse * vec4(position, 1)).xyz;
    vec3 normal = texture(gNormal, TexCoord).xyz;
	vec3 normalWorld = mat3(viewInverse) * normal;
    vec4 color = texture(gColor, TexCoord);
	
	// Reconstruct position from depth buffer
	//position = depthToViewPosition(depth, TexCoord);
	
	// Depth sensor outputs
	vec4 dscolor = texture(dsColor, TexCoord);
	float dsdepth = texture(dsDepth, TexCoord).r;
	vec3 dsposition = dsDepthToWorldPosition(dsDepth, TexCoord);
	
	
	
	
	// SSAO occlusion
	vec2 noiseScale = vec2(screenWidth / 1, screenHeight / 1);
	//vec2 noiseScale = vec2(screenWidth / 4, screenHeight / 4);
	vec3 randomVec = texture(texNoise, TexCoord * noiseScale).xyz;
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);
	
	// Transform samples from tangent to view space
	float occlusion = 0.0;
	for(int i = 0; i < kernelSize; ++i)
	{
		vec3 fsample = TBN * samples[i];
		//vec3 fsample = samples[i];
		fsample = position + fsample * kernelRadius;
		
		// view space offset vectors to window space
        vec4 offset = kinectProjection * vec4(fsample, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;
		
		float sampleDepth = texture(gPosition, offset.xy).z;
		float rangeCheck = smoothstep(0.0, 1.0, kernelRadius / abs(position.z - sampleDepth));
		occlusion += (sampleDepth >= fsample.z + sampleBias ? 1.0 : 0.0) * rangeCheck;
	}
	occlusion = 1.0 - (occlusion / kernelSize);
	
	vec4 finalColor = vec4(0, 0, 0, 1);
	finalColor.rgb = render(positionWorld, normalWorld, color, occlusion);
	
	
	
	switch (displayMode)
	{
		case 1:
			outColor = finalColor;
			break;
		
		case 2:
			outColor = vec4(position, 1);
			break;
			
		case 3:
			outColor = vec4(normal, 1);
			break;
			
		case 4:
			outColor = vec4(vec3(occlusion), 1);
			break;
			
		case 5:
			outColor = vec4(color);
			break;
			
		case 6:
			outColor = dscolor;
			break;
			
		case 7:
			outColor = vec4(dsdepth);
			break;
			
		case 8:
			outColor = vec4(dsposition, 1);
			break;
	}
}