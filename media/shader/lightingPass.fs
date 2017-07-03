#version 450

in vec2 TexCoord;
in vec3 LightPosition;

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
	float z = texture(samplerDepth, texcoord).r;
	float x = (texcoord.x - 0.5) * z * fx;
	float y = (0.5 - texcoord.y) * z * fy;
	return vec3(x, y, z);
}

vec4 circle(vec2 pos, float radius, vec3 color)
{
	vec2 vd = pos - TexCoord;
	vd.x *=  screenWidth / screenHeight;
	return vec4(color, step(length(vd), radius));
}

const float nearPlane = 0.1;
const float farPlane = 100.0;

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane)) / farPlane;
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
	if (inColor.a < 1.0 || P.rgb == vec3(0.2))
	{
		//if (inColor.a == vec3(0)) ao = 1;
		color = vec3(ao);
	}
	
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
	
	// Depth sensor textures
	vec2 dsTexCoord = vec2(TexCoord.x, 1.0 - TexCoord.y);
	vec4 dscolor = texture(dsColor, dsTexCoord);
	float dsdepth = texture(dsDepth, dsTexCoord).r;
	
	// Reconstructed position from depth (kinect)
	//position = depthToViewPosition(depth, TexCoord);
	vec3 dsposition = dsDepthToWorldPosition(dsDepth, dsTexCoord);
	//dsposition = (view * vec4(dsposition, 1)).xyz;
	
	// Blend positions
	if (color.a < 0.001) position = dsposition;
	
	// SSAO occlusion
	vec2 noiseScale = vec2(screenWidth / 4, screenHeight / 4);
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
		
		vec4 offset = vec4(fsample, 1.0);
        offset = projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
		
		float sampleDepth = texture(gPosition, offset.xy).z;
		//float sampleDepthDS = dsDepthToWorldPosition(dsDepth, offset.xy).z;
		//sampleDepth = mix(sampleDepthDS, sampleDepth, color.a);
		float rangeCheck = smoothstep(0.0, 1.0, kernelRadius / abs(position.z - sampleDepth));
		occlusion += (sampleDepth >= fsample.z + sampleBias ? 1.0 : 0.0) * rangeCheck;
	}
	occlusion = 1.0 - (occlusion / kernelSize);
	
	color.rgb = render(positionWorld, normalWorld, color, occlusion);
	
	if (color.a < 0.001) color.rgb = dscolor.rgb;
	if (color.a > 0.0 && color.a < 1.0) color.rgb = dscolor.rgb - (1 - color.rgb);
	//if (position.rgb == vec3(0.2)) position = dsposition;
	//position = mix(dsposition, position, color.a);
	
	
	// Test
	// draw a small disk at each position
	/*const int samps = 40;
	for (int i = 0; i < samps; i++)
	{
		for (int j = 0; j < samps; j++)
		{
			vec2 sampleCoord = vec2(i, j) / samps;
			vec4 circlePos = vec4(sampleCoord * 10, texture(dsDepth, vec2(sampleCoord.x, 1.0 - sampleCoord.y)).r * 10, 1);
			circlePos = projection * view * circlePos;
			circlePos.xyz = circlePos.xyz / circlePos.w;
			circlePos.xy = (circlePos.xy + 1) * 0.5;
			
			if (circlePos.w > 0) // draw circle if in front of camera
			{
				vec4 layer2 = circle(circlePos.xy, 0.01, texture(dsColor, vec2(sampleCoord.x, 1.0 - sampleCoord.y)).rgb);
				color = mix(color, layer2, layer2.a);
			}
		}
	}*/
	
	
	
	switch (displayMode)
	{
		case 1:
			outColor = color;
			break;
		
		case 2:
			outColor = vec4(position, 1);
			break;
			
		case 3:
			outColor = vec4(occlusion);
			break;
			
		case 4:
			outColor = dscolor;
			break;
			
		case 5:
			outColor = vec4(dsdepth);
			break;
			
		case 6:
			outColor = dscolor * occlusion;
			break;
	}
}