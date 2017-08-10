#pragma once

#include "global.h"
#include "Quad.h"

class PBR
{

private:

	Object* cube;
	Quad* quad;

	const int captureWidth = 1024;
	const int captureHeight = 1024;
	const int irrdianceWidth = 32;
	const int irrdianceHeight = 32;
	const int prefilterWidth = 1024;
	const int prefilterHeight = 1024;
	const int brdfLUTWidth = 512;
	const int brdfLUTHeight = 512;

	int windowWidth, windowHeight;

	Shader* hdrToCubemapShader;
	Shader* hdrIrradianceShader;
	Shader* prefilterShader;
	Shader* brdfShader;
	Shader* reflectanceShader;

	GLuint captureFBO;
	GLuint captureRBO;

	GLuint environmentMap;
	GLuint irradianceMap;
	GLuint prefilterMap;
	GLuint brdfLUT;
	GLuint reflectanceMap;

public:

	PBR(int windowWidth, int windowHeight);
	~PBR();

	void recompileShaders();
	void computeEnvMaps();
	void computeReflectanceMap(GLuint normalMap, GLuint colorMap);

	GLuint getEnvironmentMapId() const;
	GLuint getIrradianceMapId() const;
	GLuint getPrefilterMapId() const;
	GLuint getBrdfLUTId() const;
	GLuint getReflectanceMapId() const;

};