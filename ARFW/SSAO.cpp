#include "SSAO.h"

using namespace std;
using namespace glm;

SSAO::SSAO(int width, int height)
{
	texWidth = width;
	texHeight = height;
	initialize();
}

SSAO::~SSAO()
{

}

void SSAO::initialize()
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

	for (int i = 0; i < (int)(texWidth * texHeight); i++)
	{
		vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f);
		ssaoNoise.push_back(noise);
	}

	glGenTextures(1, &noiseTexId);
	glBindTexture(GL_TEXTURE_2D, noiseTexId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, texWidth, texHeight, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	quad = new Quad();
	shader = new Shader("ssao");
	recompileShader();

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, texWidth, texHeight, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSAO::recompileShader()
{
	shader->recompile();
	shader->apply();
	glUniform1i(glGetUniformLocation(shader->getShaderId(), "gPosition"), 0);
	glUniform1i(glGetUniformLocation(shader->getShaderId(), "gNormal"), 1);
	glUniform1i(glGetUniformLocation(shader->getShaderId(), "inNoise"), 2);
	glUniform3fv(glGetUniformLocation(shader->getShaderId(), "inSamples"), 64, value_ptr(kernel[0]));
}

void SSAO::draw(GLuint positionMapId, GLuint normalMapId)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, texWidth, texHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, positionMapId);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normalMapId);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, noiseTexId);

	quad->draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint SSAO::getTextureId() const
{
	return textureId;
}

float SSAO::getKernelRadius() const
{
	return kernelRadius;
}

float SSAO::getSampleBias() const
{
	return sampleBias;
}

float SSAO::getIntensity() const
{
	return intensity;
}

float SSAO::getPower() const
{
	return power;
}

void SSAO::setKernelRadius(float value)
{
	kernelRadius = value;

	shader->apply();
	glUniform1f(glGetUniformLocation(shader->getShaderId(), "kernelRadius"), kernelRadius);
}

void SSAO::setSampleBias(float value)
{
	sampleBias = value;

	shader->apply();
	glUniform1f(glGetUniformLocation(shader->getShaderId(), "sampleBias"), sampleBias);
}

void SSAO::setIntensity(float value)
{
	intensity = value;

	shader->apply();
	glUniform1f(glGetUniformLocation(shader->getShaderId(), "intensity"), intensity);
}

void SSAO::setPower(float value)
{
	power = value;

	shader->apply();
	glUniform1f(glGetUniformLocation(shader->getShaderId(), "power"), power);
}