#include "PBR.h"

using namespace std;

extern string g_ExePath;

PBR::PBR(int windowWidth, int windowHeight)
{
	this->windowWidth = windowWidth;
	this->windowHeight = windowHeight;

	cube = new Object();
	cube->load("cube/cube.obj");
	quad = new Quad();

	imgToCubemapShader = new Shader("2dToCube");
	hdrToCubemapShader = new Shader("hdrToCube");
	hdrIrradianceShader = new Shader("hdrIrradianceCube");
	prefilterShader = new Shader("hdrPrefilterCube");
	brdfShader = new Shader("brdf");
	reflectanceShader = new Shader("reflectance");

	//hdrRadiance = Image::loadHDRI(g_ExePath + "../../media/hdr/Wooden_Door/WoodenDoor_Ref.hdr");
	//hdrRadiance = Image::loadTexture(g_ExePath + "../../media/hdr/bg.png"); // remember to use imgToCubemapShader

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	glGenFramebuffers(1, &captureFBO);
	glGenRenderbuffers(1, &captureRBO);

	glGenTextures(1, &environmentMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, environmentMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, captureWidth, captureHeight, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenTextures(1, &irradianceMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, irrdianceWidth, irrdianceHeight, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenTextures(1, &prefilterMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, prefilterWidth, prefilterHeight, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glGenTextures(1, &brdfLUT);
	glBindTexture(GL_TEXTURE_2D, brdfLUT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, brdfLUTWidth, brdfLUTHeight, 0, GL_RG, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenTextures(1, &reflectanceMap);
	glBindTexture(GL_TEXTURE_2D, reflectanceMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

PBR::~PBR()
{
	delete cube;
}

void PBR::recompileShaders()
{
	imgToCubemapShader->recompile();
	/*hdrToCubemapShader->recompile();
	hdrIrradianceShader->recompile();
	prefilterShader->recompile();
	brdfShader->recompile();*/

	reflectanceShader->recompile();
}

void PBR::recomputeEnvMaps(GLuint colorMap)
{
	hdrRadiance = colorMap;
	imgToCubemapShader->apply();
	glUniform1i(glGetUniformLocation(imgToCubemapShader->getShaderId(), "recapture"), 0);

	computeEnvMaps();
}

void PBR::recomputeBothSums(GLuint colorMap)
{
	recomputeEnvMaps(colorMap);
	computeBRDFLut();
}

void PBR::computeEnvMaps()
{
	// Initial GL states
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

	// Load and convert HDR equirectangular map to cubemap texture
	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] =
	{
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};


	// pbr: convert HDR equirectangular environment map to cubemap equivalent
	imgToCubemapShader->apply();
	glUniform1i(glGetUniformLocation(imgToCubemapShader->getShaderId(), "colorMap"), 0);
	glUniformMatrix4fv(glGetUniformLocation(imgToCubemapShader->getShaderId(), "projection"), 1, GL_FALSE, value_ptr(captureProjection));
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hdrRadiance);

	glViewport(0, 0, captureWidth, captureHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glUniformMatrix4fv(glGetUniformLocation(imgToCubemapShader->getShaderId(), "view"), 1, GL_FALSE, value_ptr(captureViews[i]));
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, environmentMap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cube->drawMeshOnly();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindTexture(GL_TEXTURE_CUBE_MAP, environmentMap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	// pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
	
	

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

	// pbr: solve diffuse integral by convolution to create an irradiance (cube)map.

	hdrIrradianceShader->apply();
	glUniform1i(glGetUniformLocation(hdrIrradianceShader->getShaderId(), "environmentMap"), 0);
	glUniformMatrix4fv(glGetUniformLocation(hdrIrradianceShader->getShaderId(), "projection"), 1, GL_FALSE, value_ptr(captureProjection));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, environmentMap);

	glViewport(0, 0, irrdianceWidth, irrdianceHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glUniformMatrix4fv(glGetUniformLocation(hdrIrradianceShader->getShaderId(), "view"), 1, GL_FALSE, value_ptr(captureViews[i]));
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cube->drawMeshOnly();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.

	prefilterShader->apply();
	glUniform1i(glGetUniformLocation(prefilterShader->getShaderId(), "environmentMap"), 0);
	glUniformMatrix4fv(glGetUniformLocation(prefilterShader->getShaderId(), "projection"), 1, GL_FALSE, value_ptr(captureProjection));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, environmentMap);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	unsigned int maxMipLevels = 5;
	for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
	{
		// reisze framebuffer according to mip-level size.
		unsigned int mipWidth = (unsigned int)(prefilterWidth * std::pow(0.5f, mip));
		unsigned int mipHeight = (unsigned int)(prefilterHeight * std::pow(0.5f, mip));
		glViewport(0, 0, mipWidth, mipHeight);

		float roughness = (float)mip / (float)(maxMipLevels - 1);
		glUniform1f(glGetUniformLocation(prefilterShader->getShaderId(), "roughness"), roughness);
		for (unsigned int i = 0; i < 6; ++i)
		{
			glUniformMatrix4fv(glGetUniformLocation(prefilterShader->getShaderId(), "view"), 1, GL_FALSE, value_ptr(captureViews[i]));
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			cube->drawMeshOnly();
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PBR::computeBRDFLut()
{
	// pbr: generate a 2D LUT from the BRDF equations used.
	// then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUT, 0);

	brdfShader->apply();
	glViewport(0, 0, brdfLUTWidth, brdfLUTHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	quad->draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PBR::computeReflectanceMap(GLuint normalMap, GLuint colorMap)
{
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectanceMap, 0);

	glViewport(0, 0, windowWidth, windowHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	reflectanceShader->apply();
	glUniform1i(glGetUniformLocation(reflectanceShader->getShaderId(), "gNormal"), 0);
	glUniform1i(glGetUniformLocation(reflectanceShader->getShaderId(), "inColor"), 1);
	glUniform1i(glGetUniformLocation(reflectanceShader->getShaderId(), "envMap"), 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, normalMap);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, colorMap);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

	
	quad->draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint PBR::getEnvironmentMapId() const
{
	return environmentMap;
}

GLuint PBR::getIrradianceMapId() const
{
	return irradianceMap;
}

GLuint PBR::getPrefilterMapId() const
{
	return prefilterMap;
}

GLuint PBR::getBrdfLUTId() const
{
	return brdfLUT;
}

GLuint PBR::getReflectanceMapId() const
{
	return reflectanceMap;
}