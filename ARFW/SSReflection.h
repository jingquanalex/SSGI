#pragma once

#include "global.h"
#include "Quad.h"

class SSReflection
{

private:

	float maxSteps = 200.0f;
	float binarySearchSteps = 20.0f;
	float maxRayTraceDistance = 1.0f;
	float nearPlaneZ = -0.01f;
	float rayZThickness = 0.01f;
	float stride = 5.0f;
	float strideZCutoff = 1.0f;
	float jitterFactor = 0.5f;

	float screenEdgeFadeStart = 0.8f;
	float cameraFadeStart = 0.98f;
	float cameraFadeLength = 0.01f;

	int maxMipLevels = 5;
	int gaussianKernelRadius = 5;
	float gaussianSigma = 5.0f;
	float gaussianBSigma = 10.1f;

	float coneTraceMipLevel = 0;
	
	Quad* quad;
	Shader* ssReflectionPassShader;
	Shader* gaussianBlurShader;
	Shader* coneTraceShader;
	GLuint fbo;
	GLuint cReflection, cReflectionRay, cAmbientOcclusion;
	GLuint cLightFilterH, cLightFilterV;
	GLuint cLightFilterH2, cLightFilterV2;
	std::vector<float> gaussianKernel;
	int bufferWidth, bufferHeight;
	int cFilterWidth, cFilterHeight;

	float normpdf(float x, float s);
	void computeGaussianKernel();

public:

	SSReflection(int width, int height);
	~SSReflection();

	void draw(GLuint texPosition, GLuint texNormal, GLuint texLight, GLuint irrEnv, GLuint prefiltEnv, GLuint outTexture, GLuint& outAO);

	void initializeShaders();
	void recompileShaders();

	void setMaxSteps(float value);
	void setBinarySearchSteps(float value);
	void setMaxRayTraceDistance(float value);
	void setNearPlaneZ(float value);
	void setRayZThickness(float value);
	void setStride(float value);
	void setStrideZCutoff(float value);
	void setJitterFactor(float value);
	void setScreenEdgeFadeStart(float value);
	void setCameraFadeStart(float value);
	void setCameraFadeLength(float value);
	void setMaxMipLevels(int value);
	void setGaussianKernelRadius(int value);
	void setGaussianSigma(float value);
	void setGaussianBSigma(float value);
	void setConeTraceMipLevel(float value);
	void setRoughness(float value);

	float getMaxSteps() const;
	float getBinarySearchSteps() const;
	float getMaxRayTraceDistance() const;
	float getNearPlaneZ() const;
	float getRayZThickness() const;
	float getStride() const;
	float getStrideZCutoff() const;
	float getJitterFactor() const;
	float setScreenEdgeFadeStart() const;
	float setCameraFadeStart() const;
	float setCameraFadeLength() const;
	int getMaxMipLevels() const;
	int getGaussianKernelRadius() const;
	float getGaussianSigma() const;
	float getGaussianBSigma() const;
	float getConeTraceMipLevel() const;
};