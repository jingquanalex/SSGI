#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 outColor;

uniform int displayMode = 1;

uniform sampler2D inColor;
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColor;
uniform sampler2D aoMap;
uniform sampler2D dsColor;
uniform sampler2D dsDepth;
uniform sampler2D aoMap2;

uniform sampler2D tex1;

uniform vec2 bufferSize = vec2(1920, 1080);
uniform vec2 texelSize = 1.0 / vec2(1920, 1080);

float RadicalInverse_VdC(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

// Alchemy AO random disc samples
vec2 randomSamples(int sampleNumber, int samples)
{
	float radius = float(sampleNumber + 0.5) * (1.0 / samples);
	ivec2 ssC = ivec2(bufferSize * TexCoord);
	float hash = (3 * ssC.x ^ ssC.y + ssC.x * ssC.y) * 10.0;
	float angle = radius * hash;

	return radius * vec2(cos(angle), sin(angle));
}

void main()
{
	// Visualize sampling points
	/*vec4 color = vec4(0, 0, 0, 0);
	int samples = 13;
	vec2 point = vec2(0);
	for (int i = 0; i < samples; i++)
	{
		point = randomSamples(i, samples);
		
		vec2 seq = Hammersley(i, samples);
		float theta = 2.0 * 3.14159265 * seq.y;
		//point = vec2(seq.x * cos(theta), seq.x * sin(theta));
		
		//point = point * 2 - 1;
		point.x *= 1080.0 / 1920;
		point *= 0.5;
		point = point * 0.5 + 0.5;
		if (distance(point, TexCoord) < 0.01)
		{
			color = vec4(1, 0, 0, 0);
		}
	}
	outColor = color;*/
	
    vec4 color = texture(inColor, TexCoord);
	vec3 position = texture(gPosition, TexCoord).xyz;
	vec3 normal = texture(gNormal, TexCoord).xyz;
	vec3 albedo = texture(gColor, TexCoord).xyz;
	vec3 ao = texture(aoMap, TexCoord).rgb;
	
	// Depth sensor outputs
	vec4 dscolor = texture(dsColor, TexCoord);
	float dsdepth = texture(dsDepth, TexCoord).r;
	
	vec3 texture1 = texture(tex1, TexCoord).rgb;
	vec3 ao2 = texture(aoMap2, TexCoord).rgb;
	
	switch (displayMode)
	{
		case 1:
			outColor = color;
			break;
		
		case 2:
			outColor = vec4(position, 1);
			break;
			
		case 3:
			outColor = vec4(normal, 1);
			break;
			
		case 4:
			outColor = vec4(ao, 1);
			break;
			
		case 5:
			outColor = vec4(albedo, 1);
			break;
			
		case 6:
			outColor = vec4(dscolor.rgb, 1);
			break;
			
		case 7:
			outColor = vec4(vec3(dsdepth), 1);
			break;
			
		case 8:
			outColor = vec4(texture1, 1);
			break;
			
		case 9:
			outColor = vec4(ao2, 1);
			break;
	}
	
	//outColor = color;
}