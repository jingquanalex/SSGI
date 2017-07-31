#include "SSReflection.h"

SSReflection::SSReflection(int width, int height)
{
	quad = new Quad();

	ssReflectionPassShader = new Shader("reflectPass");
	gaussianBlurShader = new Shader("gaussianBlur");
	coneTraceShader = new Shader("coneTrace");

	initializeShaders();

	bufferWidth = width;
	bufferHeight = height;
	cFilterWidth = width;
	cFilterHeight = height;
	computeGaussianKernel();

	glGenTextures(1, &cReflection);
	glBindTexture(GL_TEXTURE_2D, cReflection);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufferWidth, bufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &cLightFilterH);
	glBindTexture(GL_TEXTURE_2D, cLightFilterH);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cFilterWidth, cFilterHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glGenerateMipmap(GL_TEXTURE_2D);

	glGenTextures(1, &cLightFilterV);
	glBindTexture(GL_TEXTURE_2D, cLightFilterV);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cFilterWidth, cFilterHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glGenerateMipmap(GL_TEXTURE_2D);
}

SSReflection::~SSReflection()
{
}

void SSReflection::draw(GLuint texPosition, GLuint texNormal, GLuint texColor, GLuint dsColor, GLuint outTexture)
{
	// Screen space reflection pass
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cReflection, 0);

	glViewport(0, 0, bufferWidth, bufferHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ssReflectionPassShader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texColor);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, dsColor);

	quad->draw();

	// Blur the light reflection buffer at each mip level
	gaussianBlurShader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cReflection);

	for (int mip = 0; mip < cFilterMaxMipLevels; mip++)
	{
		glUniform1i(glGetUniformLocation(gaussianBlurShader->getShaderId(), "mipLevel"), mip);

		// reisze framebuffer according to mip-level size.
		int mipWidth = (int)(cFilterWidth * std::pow(0.5f, mip));
		int mipHeight = (int)(cFilterHeight * std::pow(0.5f, mip));

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cLightFilterH, mip);
		glViewport(0, 0, mipWidth, mipHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUniform1i(glGetUniformLocation(gaussianBlurShader->getShaderId(), "isVertical"), 0);

		if (mip != 0)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, cLightFilterH);
		}

		quad->draw();

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cLightFilterV, mip);
		glViewport(0, 0, mipWidth, mipHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUniform1i(glGetUniformLocation(gaussianBlurShader->getShaderId(), "isVertical"), 1);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cLightFilterH);

		quad->draw();
	}

	// Cone tracing
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outTexture, 0);

	glViewport(0, 0, bufferWidth, bufferHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	coneTraceShader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texColor);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, cLightFilterV);

	quad->draw();
}

void SSReflection::initializeShaders()
{
	ssReflectionPassShader->apply();
	glUniform1i(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "gPosition"), 0);
	glUniform1i(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "gNormal"), 1);
	glUniform1i(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "inColor"), 2);
	glUniform1i(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "dsColor"), 3);

	computeGaussianKernel();
	gaussianBlurShader->apply();
	glUniform1i(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "inColor"), 0);

	coneTraceShader->apply();
	glUniform1i(glGetUniformLocation(coneTraceShader->getShaderId(), "inColor"), 0);
	glUniform1i(glGetUniformLocation(coneTraceShader->getShaderId(), "inLight"), 1);
}

void SSReflection::recompileShaders()
{
	ssReflectionPassShader->recompile();
	gaussianBlurShader->recompile();
	coneTraceShader->recompile();

	initializeShaders();
}

float SSReflection::normpdf(float x, float s)
{
	return 1 / (s * s * 2 * 3.14159265f) * exp(-x * x / (2 * s * s)) / s;
}

void SSReflection::computeGaussianKernel()
{
	gaussianKernel = std::vector<float>(gaussianKernelRadius * 2 + 1);

	for (int i = 0; i <= gaussianKernelRadius; i++)
	{
		float value = normpdf((float)i, gaussianSigma);
		gaussianKernel.at(gaussianKernelRadius + i) = gaussianKernel.at(gaussianKernelRadius - i) = value;
	}

	gaussianBlurShader->apply();
	glUniform1i(glGetUniformLocation(gaussianBlurShader->getShaderId(), "kernelRadius"), gaussianKernelRadius);
	glUniform1fv(glGetUniformLocation(gaussianBlurShader->getShaderId(), "kernel"), gaussianKernelRadius * 2 + 1, &gaussianKernel[0]);
}

void SSReflection::setGaussianKernelRadius(int value)
{
	gaussianKernelRadius = value;
	computeGaussianKernel();
}

void SSReflection::setGaussianSigma(float value)
{
	gaussianSigma = value;
	computeGaussianKernel();
}

int SSReflection::getGaussianKernelRadius() const
{
	return gaussianKernelRadius;
}

float SSReflection::getGaussianSigma() const
{
	return gaussianSigma;
}