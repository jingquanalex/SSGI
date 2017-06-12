#version 450

in vec2 Texcoord;

out vec4 outColor;

layout (std140, binding = 0) uniform MatCam
{
    mat4 projection;
    mat4 view;
};

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColor;
uniform sampler2D gDepth;
uniform vec3 eyePos;
uniform float screenWidth;
uniform float screenHeight;

const int kernelSize = 64;
const float kernelRadius = 0.5f;
const float bias = 0.025;
const vec2 noiseScale = vec2(screenWidth / 4, screenHeight / 4);
uniform sampler2D texNoise;
uniform vec3 samples[kernelSize];

void main()
{
	// Transform position and normal to view space
	vec3 fragPos = (view * texture(gPosition, Texcoord)).xyz;
    vec3 normal = (view * texture(gNormal, Texcoord)).xyz;
    vec3 color = texture(gColor, Texcoord).rgb;
    float alpha = texture(gColor, Texcoord).a;
	float depth = texture(gDepth, Texcoord).r;
	
	vec3 randomVec = texture(texNoise, Texcoord * noiseScale).xyz;
	
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);
	
	// Transform samples from tangent to view space
	float occlusion = 0.0;
	for(int i = 0; i < kernelSize; ++i)
	{
		vec3 fsample = TBN * samples[i];
		fsample = fragPos + fsample * kernelRadius; 
		
		vec4 offset = vec4(fsample, 1.0);
        offset = projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
		
		float sampleDepth = texture(gPosition, offset.xy).z;
		occlusion += (sampleDepth >= fsample.z + bias ? 1.0 : 0.0);
	}
	occlusion = 1.0 - (occlusion / kernelSize);
	
	//outColor = vec4(color, 1) * occlusion;
	outColor = vec4(color, 1);
}