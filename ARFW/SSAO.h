#pragma once

#include "global.h"
#include "Quad.h"
#include "Shader.h"
#include <random>

class SSAO
{

private:

	float kernelRadius = 0.5f;
	float sampleBias = 0.0f;

	std::vector<glm::vec3> kernel;
	GLuint noiseTexId;
	GLuint fbo;
	Quad* quad;
	Shader* shader;
	GLuint texWidth, texHeight;
	GLuint textureId;

public:

	SSAO(int width, int height);
	~SSAO();

	void initialize();
	void recompileShader();
	void draw(GLuint positionMapId, GLuint normalMapId);

	GLuint getTextureId() const;
	float getKernelRadius() const;
	float getSampleBias() const;

	void setKernelRadius(float value);
	void setSampleBias(float value);


};