#version 450

in vec2 TexCoord;

layout (location = 0) out vec4 outColor;

uniform sampler2D inColor;

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
	
	outColor = texture(inColor, TexCoord);
}