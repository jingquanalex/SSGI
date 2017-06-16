#pragma once

#include "global.h"
#include <OpenNI2\OpenNI.h>

const GLuint maxDepth = 10000;

class DSensor
{

private:

	bool hasError = false, initOk = false;
	openni::Device device;
	openni::VideoStream depthStream, colorStream;
	openni::VideoStream** streams;
	GLuint texWidth, texHeight;
	openni::RGB888Pixel* texColorMap, * texDepthMap;
	GLuint texSize;

	GLuint gltexColorMap, gltexDepthMap;

	GLuint minNumChunks(GLuint dataSize, GLuint chunkSize);
	GLuint minChunkSize(GLuint dataSize, GLuint chunkSize);
	void calculateHistogram(float* pHistogram, int histogramSize, const openni::VideoFrameRef& frame);

public:

	DSensor();
	~DSensor();

	void initialize(GLuint texSize);
	void render();
	GLuint getColorMapId() const;
	GLuint getDepthMapId() const;

};