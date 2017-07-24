#pragma once

#include "global.h"
#include "Quad.h"
#include "Shader.h"
#include <random>

class SSAO
{

private:

	float kernelRadius = 0.05f;
	float sampleBias = 0.001f;
	float intensity = 1.0f;
	float power = 1.0f;

	std::vector<glm::vec3> kernel;
	GLuint noiseTexId;
	GLuint fbo, fbo2;
	Quad* quad;
	Shader* shader, * mixLayerShader;
	GLuint texWidth, texHeight;
	GLuint texCombined, texLayer1, texLayer2;

	GLuint positionMapId, normalMapId, colorMapId;

public:

	SSAO(int width, int height);
	~SSAO();

	void initializeShaders();
	void recompileShaders();
	void drawLayer(int layer, GLuint positionMapId, GLuint normalMapId, GLuint colorMapId);
	void drawCombined(GLuint colorMapId);

	GLuint getTextureLayer(int layer) const;
	float getKernelRadius() const;
	float getSampleBias() const;
	float getIntensity() const;
	float getPower() const;

	void setKernelRadius(float value);
	void setSampleBias(float value);
	void setIntensity(float value);
	void setPower(float value);


};