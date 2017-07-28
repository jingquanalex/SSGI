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

// Helper functions

vec3 depthToViewPosition(float depth, vec2 texcoord)
{
    vec4 clipPosition = vec4(texcoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPosition = projectionInverse * clipPosition;
    return (viewPosition / viewPosition.w).xyz;
}

vec3 kinectDepthToViewPosition(float depth, vec2 texcoord)
{
    vec4 clipPosition = vec4(texcoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPosition = kinectProjectionInverse * clipPosition;
    return (viewPosition / viewPosition.w).xyz;
}

const float fx = 1; // 640 / (2*tan(deg2rad(61.9999962) / 2)) = 532.5695
const float fy = 1; // 480 / (2*tan(deg2rad(48.5999985) / 2)) = 531.5411
const float cx = 640 / 2;
const float cy = 480 / 2;

vec3 kinectDepthToViewPosition2(float depth, vec2 texcoord)
{
	float z = depth / 1000;
	float x = (texcoord.x * 640 - cx) / fx * depth;
	float y = (texcoord.y * 480 - cy) / fy * depth;
	return vec3(x, y, z);
}

// Raymarching stuff
/*
vec2 opU(vec2 d1, vec2 d2)
{
	return (d1.x < d2.x) ? d1 : d2;
}

float sdSphere(vec3 pos, float size)
{
    return length(pos) - size;
}

float sdPlane(vec3 pos)
{
	return pos.y;
}

vec2 scene(vec3 pos)
{
    vec2 res = opU(vec2(sdPlane(pos), 1), vec2(sdSphere(pos - vec3(0.0, 0.25, 0.0), 0.25), 86.9));
    return res;
}

vec2 castRay(vec3 ro, vec3 rd)
{
    float tmin = 0.1;
    float tmax = 1000.0;
    float t = tmin;
    float m = -1.0;
	
    for(int i = 0; i < 64; i++)
    {
	    float vprecision = 0.0005 * t;
	    vec2 res = scene(ro + rd * t);
        if(res.x < vprecision || t > tmax) break;
        t += res.x;
	    m = res.y;
    }

    if(t > tmax) m = -1.0;
    return vec2(t, m);
}

vec3 render(vec3 ro, vec3 rd)
{ 
    vec3 col = vec3(0.7, 0.9, 1.0) + rd.y * 0.8;
    vec2 res = castRay(ro,rd);
    float t = res.x;
	float m = res.y;
    if(m > -0.5)
    {
        vec3 pos = ro + t*rd;
        
        // material        
		col = 0.45 + 0.35*sin( vec3(0.05,0.08,0.10)*(m-1.0) );
        if( m<1.5 )
        {
            
            float f = mod( floor(5.0*pos.z) + floor(5.0*pos.x), 2.0);
            col = 0.3 + 0.1*f*vec3(1.0);
        }

        
    }

	return vec3( clamp(col,0.0,1.0) );
}*/

// Main

void main()
{
	// Position and normal are view space
	vec3 position = texture(gPosition, TexCoord).xyz;
    vec3 normal = texture(gNormal, TexCoord).xyz;
    vec4 color = texture(gColor, TexCoord);
	float depth = texture(gDepth, TexCoord).r;
	
	// Raymarching stuff
	/*vec2 p = TexCoord * 2.0 - 1.0;
	vec3 ro = vec3(0, 0.1, -3);
	vec3 rd = normalize(vec3(p, 2.0));
	color.rgb = render(ro, rd);*/
	
	
	
	
	// Depth sensor textures
	vec2 dsTexcoord = vec2(TexCoord.x, 1.0 - TexCoord.y);
	vec4 dscolor = texture(dsColor, dsTexcoord);
	float dsdepth = texture(dsDepth, dsTexcoord).r;
	
	// Reconstructed position from depth (kinect)
	//position = depthToViewPosition(depth, TexCoord);
	position = kinectDepthToViewPosition(dsdepth, TexCoord);
	
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
		//vec3 fsample = TBN * samples[i];
		vec3 fsample = samples[i];
		fsample = position + fsample * kernelRadius;
		
		vec4 offset = vec4(fsample, 1.0);
        offset = kinectProjection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
		
		//float sampleDepth = texture(gPosition, offset.xy).z;
		//float sampleDepth = depthToViewPosition(texture(gDepth, offset.xy).r, offset.xy).z;
		float sampleDepth = kinectDepthToViewPosition(texture(dsDepth, offset.xy).r, offset.xy).z;
		float rangeCheck = smoothstep(0.0, 1.0, kernelRadius / abs(position.z - sampleDepth));
		occlusion += (sampleDepth >= fsample.z + sampleBias ? 1.0 : 0.0) * rangeCheck;
	}
	occlusion = 1.0 - (occlusion / kernelSize);
	
	switch (displayMode)
	{
		case 1:
			outColor = color * occlusion;
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