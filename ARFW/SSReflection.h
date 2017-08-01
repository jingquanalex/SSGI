#pragma once

#include "global.h"
#include "Quad.h"

class SSReflection
{

private:

	float maxSteps = 100.0f;
	float binarySearchSteps = 20.0f;
	float maxRayTraceDistance = 0.5f;
	float nearPlaneZ = -0.01f;
	float rayZThickness = 0.01f;
	float stride = 5.0f;
	float strideZCutoff = 1.0f;
	float jitterFactor = 0.5f;

	float screenEdgeFadeStart = 0.8f;
	float cameraFadeStart = 0.2f;
	float cameraFadeLength = 0.2f;

	int maxMipLevels = 5;
	int gaussianKernelRadius = 5;
	float gaussianSigma = 5.0f;
	
	Quad* quad;
	Shader* ssReflectionPassShader;
	Shader* gaussianBlurShader;
	Shader* coneTraceShader;
	GLuint fbo;
	GLuint cReflection, cColorTest;
	GLuint cLightFilterH, cLightFilterV;
	std::vector<float> gaussianKernel;
	int bufferWidth, bufferHeight;
	int cFilterWidth, cFilterHeight;

	float normpdf(float x, float s);
	void computeGaussianKernel();

public:

	SSReflection(int width, int height);
	~SSReflection();

	void draw(GLuint texPosition, GLuint texNormal, GLuint texLight, GLuint dsColor, GLuint outTexture);

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
};