#pragma once

#include "global.h"
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

	GLuint gltexColorMap, gltexDepthMap;

	GLuint minNumChunks(GLuint dataSize, GLuint chunkSize);
	GLuint minChunkSize(GLuint dataSize, GLuint chunkSize);
	void calculateHistogram(float* pHistogram, int histogramSize, const openni::VideoFrameRef& frame);

	glm::mat4 matProjection, matProjectionInverse;

public:

	DSensor();
	~DSensor();

	void initialize(int windowWidth, int windowHeight);
	void update();

	GLuint getColorMapId() const;
	GLuint getDepthMapId() const;
	glm::mat4 getMatProjection() const;
	glm::mat4 getMatProjectionInverse() const;
	void toggleRendering();

};