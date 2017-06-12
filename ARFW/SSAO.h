#pragma once

#include "global.h"
#include <random>

class SSAO
{

private:

	std::vector<glm::vec3> kernel;
	GLuint noiseTexId;

public:

	SSAO();
	~SSAO();

	void Initialize();
	std::vector<glm::vec3> getKernel() const;
	GLuint getNoiseTexId() const;

};