#include "SSAO.h"

using namespace std;
using namespace glm;

SSAO::SSAO(int width, int height)
{
	texWidth = width;
	texHeight = height;
	
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

	// Generate noise texture
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
	mixLayerShader = new Shader("ssaoMixLayer");
	initializeShaders();

	glGenFramebuffers(1, &fbo);

	glGenTextures(1, &texLayer1);
	glBindTexture(GL_TEXTURE_2D, texLayer1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &texLayer2);
	glBindTexture(GL_TEXTURE_2D, texLayer2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenFramebuffers(1, &fbo2);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo2);

	glGenTextures(1, &texCombined);
	glBindTexture(GL_TEXTURE_2D, texCombined);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texCombined, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

SSAO::~SSAO()
{
	delete quad;
	delete shader;
	delete mixLayerShader;
}

void SSAO::initializeShaders()
{
	shader->apply();
	glUniform1i(glGetUniformLocation(shader->getShaderId(), "gPosition"), 0);
	glUniform1i(glGetUniformLocation(shader->getShaderId(), "gNormal"), 1);
	glUniform1i(glGetUniformLocation(shader->getShaderId(), "inNoise"), 2);
	glUniform1i(glGetUniformLocation(shader->getShaderId(), "gColor"), 3);
	glUniform3fv(glGetUniformLocation(shader->getShaderId(), "inSamples"), 64, value_ptr(kernel[0]));
	glUniform1f(glGetUniformLocation(shader->getShaderId(), "kernelRadius"), kernelRadius);
	glUniform1f(glGetUniformLocation(shader->getShaderId(), "sampleBias"), sampleBias);
	glUniform1f(glGetUniformLocation(shader->getShaderId(), "intensity"), intensity);
	glUniform1f(glGetUniformLocation(shader->getShaderId(), "power"), power);

	mixLayerShader->apply();
	glUniform1i(glGetUniformLocation(mixLayerShader->getShaderId(), "layer1"), 0);
	glUniform1i(glGetUniformLocation(mixLayerShader->getShaderId(), "layer2"), 1);
	glUniform1i(glGetUniformLocation(mixLayerShader->getShaderId(), "gColor"), 2);
}

void SSAO::recompileShaders()
{
	shader->recompile();
	mixLayerShader->recompile();
	
	initializeShaders();
}

void SSAO::drawLayer(int layer, GLuint positionMapId, GLuint normalMapId, GLuint colorMapId)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	GLuint currentId;
	if (layer == 1)currentId = texLayer1;
	else currentId = texLayer2;
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, currentId, 0);

	glViewport(0, 0, texWidth, texHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, positionMapId);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normalMapId);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, noiseTexId);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, colorMapId);

	quad->draw();
}

void SSAO::drawCombined(GLuint colorMapId)
{
	// Mix ssao layer results
	glBindFramebuffer(GL_FRAMEBUFFER, fbo2);
	glViewport(0, 0, texWidth, texHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mixLayerShader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texLayer1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texLayer2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, colorMapId);

	quad->draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint SSAO::getTextureLayer(int layer) const
{
	if (layer == 0) return texCombined;
	else if (layer == 1) return texLayer1;
	else return texLayer2;
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