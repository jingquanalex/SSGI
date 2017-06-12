#include "SSAO.h"

using namespace std;
using namespace glm;

SSAO::SSAO()
{
	Initialize();
}

SSAO::~SSAO()
{

}

void SSAO::Initialize()
{
	kernel.clear();

	// Generate hemisphere sampling kernel
	uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
	default_random_engine generator;
	
	for (int i = 0; i < 64; i++)
	{
		vec3 sample(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator));
		sample = normalize(sample);
		sample *= randomFloats(generator);
		float scale = (float)i / 64.0f;
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		kernel.push_back(sample);
	}

	// Generate 4x4 noise texture
	vector<vec3> ssaoNoise;

	for (int i = 0; i < 16; i++)
	{
		vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f);
		ssaoNoise.push_back(noise);
	}

	glGenTextures(1, &noiseTexId);
	glBindTexture(GL_TEXTURE_2D, noiseTexId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);
}

std::vector<glm::vec3> SSAO::getKernel() const
{
	return kernel;
}

GLuint SSAO::getNoiseTexId() const
{
	return noiseTexId;
}