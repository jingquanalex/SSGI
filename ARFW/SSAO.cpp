#include "SSAO.h"

using namespace std;
using namespace glm;

SSAO::SSAO(int width, int height)
{
	// Half size buffers
	texWidth = width;
	texHeight = height;
	texWidthSmall = width / downscaleFactor;
	texHeightSmall = width / downscaleFactor;
	
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

	for (int i = 0; i < (int)(texWidthSmall * texHeightSmall); i++)
	{
		//vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f);
		vec3 noise(randomFloats(generator), randomFloats(generator), 0.0f);
		ssaoNoise.push_back(noise);
	}

	glGenTextures(1, &noiseTexId);
	glBindTexture(GL_TEXTURE_2D, noiseTexId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidthSmall, texHeightSmall, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);

	quad = new Quad();
	shader = new Shader("ssao");
	upsampleShader = new Shader("ssaoUpsample");
	mixLayerShader = new Shader("ssaoMixLayer");
	initializeShaders();

	glGenFramebuffers(1, &fbo);

	glGenTextures(1, &texSSAO);
	glBindTexture(GL_TEXTURE_2D, texSSAO);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidthSmall, texHeightSmall, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &texFilterH);
	glBindTexture(GL_TEXTURE_2D, texFilterH);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &texFilterV1);
	glBindTexture(GL_TEXTURE_2D, texFilterV1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &texFilterV2);
	glBindTexture(GL_TEXTURE_2D, texFilterV2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenFramebuffers(1, &fboCombined);
	glBindFramebuffer(GL_FRAMEBUFFER, fboCombined);

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
	glUniform1i(glGetUniformLocation(shader->getShaderId(), "kernelSize"), kernelSize);
	glUniform1f(glGetUniformLocation(shader->getShaderId(), "kernelRadius"), kernelRadius);
	glUniform1f(glGetUniformLocation(shader->getShaderId(), "sampleBias"), sampleBias);
	glUniform1f(glGetUniformLocation(shader->getShaderId(), "intensity"), intensity);
	glUniform1f(glGetUniformLocation(shader->getShaderId(), "power"), power);

	computeBlurKernel();
	upsampleShader->apply();
	glUniform1i(glGetUniformLocation(upsampleShader->getShaderId(), "inColor"), 0);
	glUniform1i(glGetUniformLocation(upsampleShader->getShaderId(), "inNormal"), 1);
	glUniform1f(glGetUniformLocation(upsampleShader->getShaderId(), "bsigma"), blurBSigma);

	mixLayerShader->apply();
	glUniform1i(glGetUniformLocation(mixLayerShader->getShaderId(), "layer1"), 0);
	glUniform1i(glGetUniformLocation(mixLayerShader->getShaderId(), "layer2"), 1);
	glUniform1i(glGetUniformLocation(mixLayerShader->getShaderId(), "gColor"), 2);
}

void SSAO::recompileShaders()
{
	shader->recompile();
	upsampleShader->recompile();
	mixLayerShader->recompile();
	
	initializeShaders();
}

void SSAO::drawLayer(int layer, GLuint positionMapId, GLuint normalMapId, GLuint colorMapId)
{
	// Compute ssao for each layer
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texSSAO, 0);

	glViewport(0, 0, texWidthSmall, texHeightSmall);
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

	// Upsample to full resolution (horizontal pass)
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texFilterH, 0);

	glViewport(0, 0, texWidth, texHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	upsampleShader->apply();
	glUniform1i(glGetUniformLocation(upsampleShader->getShaderId(), "isVertical"), 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texSSAO);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normalMapId);

	quad->draw();

	// (vertical pass)
	GLuint texLayer;
	if (layer == 1) texLayer = texFilterV1;
	else texLayer = texFilterV2;
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texLayer, 0);

	glViewport(0, 0, texWidth, texHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	upsampleShader->apply();
	glUniform1i(glGetUniformLocation(upsampleShader->getShaderId(), "isVertical"), 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texFilterH);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normalMapId);

	quad->draw();
}

void SSAO::drawCombined(GLuint colorMapId)
{
	// Combine the 2 ssao layer results
	glBindFramebuffer(GL_FRAMEBUFFER, fboCombined);
	glViewport(0, 0, texWidth, texHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mixLayerShader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texFilterV1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texFilterV2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, colorMapId);

	quad->draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

float SSAO::normpdf(float x, float s)
{
	return 0.3989422f * exp(-x * x / (2.0f * s * s)) / s;
}

void SSAO::computeBlurKernel()
{
	blurKernel = std::vector<float>(blurKernelRadius * 2 + 1);

	for (int i = 0; i <= blurKernelRadius; i++)
	{
		float value = normpdf((float)i, blurSigma);
		blurKernel.at(blurKernelRadius + i) = blurKernel.at(blurKernelRadius - i) = value;
	}

	upsampleShader->apply();
	glUniform1fv(glGetUniformLocation(upsampleShader->getShaderId(), "kernel"), blurKernelRadius * 2 + 1, &blurKernel[0]);
	glUniform1i(glGetUniformLocation(upsampleShader->getShaderId(), "kernelRadius"), blurKernelRadius);
	glUniform1f(glGetUniformLocation(upsampleShader->getShaderId(), "sigma"), blurSigma);
}

GLuint SSAO::getTextureLayer(int layer) const
{
	if (layer == 0) return texCombined;
	else if (layer == 1) return texFilterV1;
	else return texFilterV2;
}

int SSAO::getKernelSize() const
{
	return kernelSize;
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

int SSAO::getBlurKernelRadius() const
{
	return blurKernelRadius;
}

float SSAO::getBlurSigma() const
{
	return blurSigma;
}

float SSAO::getBlurBSigma() const
{
	return blurBSigma;
}

void SSAO::setKernelSize(int value)
{
	kernelSize = value;
	shader->apply();
	glUniform1i(glGetUniformLocation(shader->getShaderId(), "kernelSize"), kernelSize);
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

void SSAO::setBlurKernelRadius(int value)
{
	blurKernelRadius = value;
	computeBlurKernel();
}

void SSAO::setBlurSigma(float value)
{
	blurSigma = value;
	computeBlurKernel();
}

void SSAO::setBlurBSigma(float value)
{
	blurBSigma = value;
	upsampleShader->apply();
	glUniform1f(glGetUniformLocation(upsampleShader->getShaderId(), "bsigma"), blurBSigma);
}