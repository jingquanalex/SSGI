#pragma once

#include "global.h"
#include "Quad.h"

class SSReflection
{

private:

	Quad* quad;

	int bufferWidth, bufferHeight;
	float ssrMaxSteps = 100;

	int cFilterMaxMipLevels = 5;
	int gaussianKernelRadius = 55;
	float gaussianSigma = 55.0f;
	
	Shader* ssReflectionPassShader;
	Shader* gaussianBlurShader;
	Shader* coneTraceShader;
	GLuint cReflection;
	GLuint cLightFilterH, cLightFilterV;
	std::vector<float> gaussianKernel;
	int cFilterWidth, cFilterHeight;

	float normpdf(float x, float s);
	void computeGaussianKernel();

public:

	SSReflection(int width, int height);
	~SSReflection();

	void draw(GLuint texPosition, GLuint texNormal, GLuint texColor, GLuint dsColor, GLuint outTexture);

	void initializeShaders();
	void recompileShaders();

	void setGaussianKernelRadius(int value);
	void setGaussianSigma(float value);

	int getGaussianKernelRadius() const;
	float getGaussianSigma() const;
};