#pragma once

#include "global.h"
#include "Quad.h"

class PBR
{

private:

	Object* cube;
	Quad* quad;

public:

	PBR();
	~PBR();

	void precomputeEnvMaps(GLuint& envMap, GLuint& irrMap, GLuint& filtEnvMap, GLuint& brdfLUT);

};