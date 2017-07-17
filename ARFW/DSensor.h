#pragma once

#include "global.h"
#include "Quad.h"
#include "Shader.h"
#include <OpenNI2\OpenNI.h>

const GLuint maxDepth = 10000;

class DSensor
{

private:

	bool hasError = false, initOk = false, isRendering = true;;
	openni::Device device;
	openni::VideoStream depthStream, colorStream;
	openni::VideoStream** streams;
	GLuint texWidth, texHeight;
	openni::RGB888Pixel* texColorMap;
	uint16_t* texDepthMap;

	GLuint dsColorMap, dsDepthMap;
	int dsDepthMapLayers, dsDepthMapLayerCounter = 1;

	GLuint minNumChunks(GLuint dataSize, GLuint chunkSize);
	GLuint minChunkSize(GLuint dataSize, GLuint chunkSize);
	void calculateHistogram(float* pHistogram, int histogramSize, const openni::VideoFrameRef& frame);

	glm::mat4 matProjection, matProjectionInverse;

	GLuint fbo, fbo2;
	GLuint outColorMap, outDepthMap, outPositionMap, outNormalMap;
	GLuint outColorMap2, outDepthMap2, outPositionMap2, outNormalMap2;
	Quad* quad;
	Shader* temporalMedianShader;
	Shader* medianShader;
	Shader* fillShader;
	Shader* positionShader;
	Shader* blurShader;

public:

	DSensor();
	~DSensor();

	void initialize(int windowWidth, int windowHeight);
	void recompileShaders();
	void update();
	

	GLuint getColorMapId() const;
	GLuint getDepthMapId() const;
	GLuint getPositionMapId() const;
	GLuint getNormalMapId() const;
	glm::mat4 getMatProjection() const;
	glm::mat4 getMatProjectionInverse() const;
	void toggleRendering();

};