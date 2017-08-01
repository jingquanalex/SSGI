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
	maxMipLevels = 1 + (int)floor(log2(glm::max(bufferWidth, bufferHeight)));
	computeGaussianKernel();

	glGenTextures(1, &cReflection);
	glBindTexture(GL_TEXTURE_2D, cReflection);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bufferWidth, bufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &cColorTest);
	glBindTexture(GL_TEXTURE_2D, cColorTest);
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

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	const GLenum attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);
}

SSReflection::~SSReflection()
{
}

void SSReflection::draw(GLuint texPosition, GLuint texNormal, GLuint texLight, GLuint dsColor, GLuint outTexture)
{
	// Screen space reflection pass
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cReflection, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, cColorTest, 0);

	glViewport(0, 0, bufferWidth, bufferHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ssReflectionPassShader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texLight);

	quad->draw();

	// Make sure to unbind drawing to 2nd attachment
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);

	// Create a mip chain of blurred light (color) buffer
	gaussianBlurShader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texLight);

	for (int mip = 0; mip < maxMipLevels; mip++)
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
	glBindTexture(GL_TEXTURE_2D, texPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, cReflection);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, cLightFilterV);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, cColorTest);

	quad->draw();
}

void SSReflection::initializeShaders()
{
	ssReflectionPassShader->apply();
	glUniform1i(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "gPosition"), 0);
	glUniform1i(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "gNormal"), 1);
	glUniform1i(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "inColor"), 2);

	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "maxSteps"), maxSteps);
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "binarySearchSteps"), binarySearchSteps);
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "maxRayTraceDistance"), maxRayTraceDistance);
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "nearPlaneZ"), nearPlaneZ);
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "rayZThickness"), rayZThickness);
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "stride"), stride);
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "strideZCutoff"), strideZCutoff);
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "jitterFactor"), jitterFactor);
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "screenEdgeFadeStart"), screenEdgeFadeStart);
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "cameraFadeStart"), cameraFadeStart);
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "cameraFadeLength"), cameraFadeLength);

	computeGaussianKernel();
	gaussianBlurShader->apply();
	glUniform1i(glGetUniformLocation(gaussianBlurShader->getShaderId(), "inColor"), 0);

	coneTraceShader->apply();
	glUniform1i(glGetUniformLocation(coneTraceShader->getShaderId(), "gPosition"), 0);
	glUniform1i(glGetUniformLocation(coneTraceShader->getShaderId(), "gNormal"), 1);
	glUniform1i(glGetUniformLocation(coneTraceShader->getShaderId(), "inRay"), 2);
	glUniform1i(glGetUniformLocation(coneTraceShader->getShaderId(), "inLight"), 3);
	glUniform1i(glGetUniformLocation(coneTraceShader->getShaderId(), "inColorTest"), 4);
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

void SSReflection::setMaxSteps(float value)
{
	maxSteps = value;
	ssReflectionPassShader->apply();
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "maxSteps"), maxSteps);
}

void SSReflection::setBinarySearchSteps(float value)
{
	binarySearchSteps = value;
	ssReflectionPassShader->apply();
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "binarySearchSteps"), binarySearchSteps);
}

void SSReflection::setMaxRayTraceDistance(float value)
{
	maxRayTraceDistance = value;
	ssReflectionPassShader->apply();
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "maxRayTraceDistance"), maxRayTraceDistance);
}

void SSReflection::setNearPlaneZ(float value)
{
	nearPlaneZ = value;
	ssReflectionPassShader->apply();
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "nearPlaneZ"), nearPlaneZ);
}

void SSReflection::setRayZThickness(float value)
{
	rayZThickness = value;
	ssReflectionPassShader->apply();
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "rayZThickness"), rayZThickness);
}

void SSReflection::setStride(float value)
{
	stride = value;
	ssReflectionPassShader->apply();
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "stride"), stride);
}

void SSReflection::setStrideZCutoff(float value)
{
	strideZCutoff = value;
	ssReflectionPassShader->apply();
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "strideZCutoff"), strideZCutoff);
}

void SSReflection::setJitterFactor(float value)
{
	jitterFactor = value;
	ssReflectionPassShader->apply();
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "jitterFactor"), jitterFactor);
}

void SSReflection::setScreenEdgeFadeStart(float value)
{
	screenEdgeFadeStart = value;
	ssReflectionPassShader->apply();
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "screenEdgeFadeStart"), screenEdgeFadeStart);
}

void SSReflection::setCameraFadeStart(float value)
{
	cameraFadeStart = value;
	ssReflectionPassShader->apply();
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "cameraFadeStart"), cameraFadeStart);
}

void SSReflection::setCameraFadeLength(float value)
{
	cameraFadeLength = value;
	ssReflectionPassShader->apply();
	glUniform1f(glGetUniformLocation(ssReflectionPassShader->getShaderId(), "cameraFadeLength"), cameraFadeLength);
}

void SSReflection::setMaxMipLevels(int value)
{
	maxMipLevels = value;
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

float SSReflection::getMaxSteps() const
{
	return maxSteps;
}

float SSReflection::getBinarySearchSteps() const
{
	return binarySearchSteps;
}

float SSReflection::getMaxRayTraceDistance() const
{
	return maxRayTraceDistance;
}

float SSReflection::getNearPlaneZ() const
{
	return nearPlaneZ;
}

float SSReflection::getRayZThickness() const
{
	return rayZThickness;
}

float SSReflection::getStride() const
{
	return stride;
}

float SSReflection::getStrideZCutoff() const
{
	return strideZCutoff;
}

float SSReflection::getJitterFactor() const
{
	return jitterFactor;
}

float SSReflection::setScreenEdgeFadeStart() const
{
	return screenEdgeFadeStart;
}

float SSReflection::setCameraFadeStart() const
{
	return cameraFadeStart;
}

float SSReflection::setCameraFadeLength() const
{
	return cameraFadeLength;
}

int SSReflection::getMaxMipLevels() const
{
	return maxMipLevels;
}

int SSReflection::getGaussianKernelRadius() const
{
	return gaussianKernelRadius;
}

float SSReflection::getGaussianSigma() const
{
	return gaussianSigma;
}