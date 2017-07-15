#include "DSensor.h"

DSensor::DSensor()
{

}

DSensor::~DSensor()
{
	if (initOk)
	{
		depthStream.stop();
		colorStream.stop();
		depthStream.destroy();
		colorStream.destroy();
		device.close();
		openni::OpenNI::shutdown();
	}
}

void DSensor::initialize(int windowWidth, int windowHeight)
{
	printf("Initializing OpenNI:\n");

	openni::Status rc = openni::OpenNI::initialize();
	if (rc != openni::STATUS_OK)
	{
		printf("OpenNI Initialization Error: \n%s\n", openni::OpenNI::getExtendedError());
		hasError = true;
		return;
	}

	rc = device.open(openni::ANY_DEVICE);

	if (rc != openni::STATUS_OK)
	{
		printf("OpenNI Error: Device open failed:\n%s\n", openni::OpenNI::getExtendedError());
		openni::OpenNI::shutdown();
		hasError = true;
		return;
	}

	rc = depthStream.create(device, openni::SENSOR_DEPTH);
	if (rc == openni::STATUS_OK)
	{
		const openni::SensorInfo* sensorInfo = device.getSensorInfo(openni::SENSOR_DEPTH);
		const openni::Array<openni::VideoMode>& supportedVideoModes = sensorInfo->getSupportedVideoModes();

		printf("Supported depth video modes: \n");
		for (int i = 0; i < supportedVideoModes.getSize(); i++)
		{
			printf("%i: %ix%i, %i fps, %i format\n", i, supportedVideoModes[i].getResolutionX(), supportedVideoModes[i].getResolutionY(),
				supportedVideoModes[i].getFps(), supportedVideoModes[i].getPixelFormat());
		}

		rc = depthStream.start();
		if (rc != openni::STATUS_OK)
		{
			printf("OpenNI Error: Couldn't start depth stream:\n%s\n", openni::OpenNI::getExtendedError());
			depthStream.destroy();
		}
	}
	else
	{
		printf("OpenNI Error: Couldn't find depth stream:\n%s\n", openni::OpenNI::getExtendedError());
	}

	rc = colorStream.create(device, openni::SENSOR_COLOR);
	if (rc == openni::STATUS_OK)
	{
		rc = colorStream.start();
		if (rc != openni::STATUS_OK)
		{
			printf("OpenNI Error: Couldn't start color stream:\n%s\n", openni::OpenNI::getExtendedError());
			colorStream.destroy();
		}
	}
	else
	{
		printf("OpenNI Error: Couldn't find color stream:\n%s\n", openni::OpenNI::getExtendedError());
	}

	openni::VideoMode depthVideoMode;
	openni::VideoMode colorVideoMode;
	GLuint videoWidth, videoHeight;

	if (depthStream.isValid() && colorStream.isValid())
	{
		depthVideoMode = depthStream.getVideoMode();
		colorVideoMode = colorStream.getVideoMode();

		int depthWidth = depthVideoMode.getResolutionX();
		int depthHeight = depthVideoMode.getResolutionY();
		int colorWidth = colorVideoMode.getResolutionX();
		int colorHeight = colorVideoMode.getResolutionY();
		int minDepth = depthStream.getMinPixelValue();
		int maxDepth = depthStream.getMaxPixelValue();

		if (depthWidth == colorWidth && depthHeight == colorHeight)
		{
			videoWidth = depthWidth;
			videoHeight = depthHeight;
		}
		else
		{
			printf("OpenNI Error: expect color and depth to be in same resolution: D: %dx%d, C: %dx%d\n", depthWidth, depthHeight, colorWidth, colorHeight);
			hasError = true;
			return;
		}
	}
	else if (depthStream.isValid())
	{
		depthVideoMode = depthStream.getVideoMode();
		videoWidth = depthVideoMode.getResolutionX();
		videoHeight = depthVideoMode.getResolutionY();
	}
	else if (colorStream.isValid())
	{
		colorVideoMode = colorStream.getVideoMode();
		videoWidth = colorVideoMode.getResolutionX();
		videoHeight = colorVideoMode.getResolutionY();
	}
	else
	{
		printf("OpenNI Error: expects at least one of the streams to be valid...\n");
		hasError = true;
		return;
	}

	streams = new openni::VideoStream*[2];
	streams[0] = &depthStream;
	streams[1] = &colorStream;

	// Create projection matrix (depth)
	float fovX = glm::degrees(depthStream.getHorizontalFieldOfView());
	float fovY = glm::degrees(depthStream.getVerticalFieldOfView());
	float w = (float)windowWidth / videoWidth;
	float h = (float)windowHeight / videoHeight;
	matProjection = glm::perspective(glm::radians(fovY), w / h, 0.01f, 1000.0f);
	matProjectionInverse = inverse(matProjection);

	// Texture map init
	//texWidth = minChunkSize(videoWidth, texSize);
	//texHeight = minChunkSize(videoHeight, texSize);
	texWidth = videoWidth;
	texHeight = videoHeight;
	texColorMap = new openni::RGB888Pixel[texWidth * texHeight];
	texDepthMap = new uint16_t[texWidth * texHeight];

	device.setImageRegistrationMode(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR);

	if (!hasError)
	{
		printf("\tOK\n");
		initOk = true;
	}

	glGenTextures(1, &dsColorMap);
	glBindTexture(GL_TEXTURE_2D, dsColorMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &dsDepthMap);
	glBindTexture(GL_TEXTURE_2D, dsDepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, texWidth, texHeight, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &dsDepthMapPrev);
	glBindTexture(GL_TEXTURE_2D, dsDepthMapPrev);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, texWidth, texHeight, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	quad = new Quad();
	fillShader = new Shader("dsFill");
	medianShader = new Shader("dsMedian");
	positionShader = new Shader("dsPosition");
	blurShader = new Shader("dsBlur");
	recompileShaders();
	
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGenTextures(1, &outColorMap);
	glBindTexture(GL_TEXTURE_2D, outColorMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outColorMap, 0);

	glGenTextures(1, &outDepthMap);
	glBindTexture(GL_TEXTURE_2D, outDepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, texWidth, texHeight, 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, outDepthMap, 0);

	glGenTextures(1, &outPositionMap);
	glBindTexture(GL_TEXTURE_2D, outPositionMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, texWidth, texHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, outPositionMap, 0);

	glGenTextures(1, &outNormalMap);
	glBindTexture(GL_TEXTURE_2D, outNormalMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, texWidth, texHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, outNormalMap, 0);

	const GLuint attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);

	glGenFramebuffers(1, &fbo2);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo2);

	glGenTextures(1, &outColorMap2);
	glBindTexture(GL_TEXTURE_2D, outColorMap2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outColorMap2, 0);

	glGenTextures(1, &outDepthMap2);
	glBindTexture(GL_TEXTURE_2D, outDepthMap2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, texWidth, texHeight, 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, outDepthMap2, 0);

	glGenTextures(1, &outPositionMap2);
	glBindTexture(GL_TEXTURE_2D, outPositionMap2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, texWidth, texHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, outPositionMap2, 0);

	glGenTextures(1, &outNormalMap2);
	glBindTexture(GL_TEXTURE_2D, outNormalMap2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, texWidth, texHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, outNormalMap2, 0);

	glDrawBuffers(4, attachments);
}

void DSensor::recompileShaders()
{
	fillShader->recompile();
	fillShader->apply();
	glUniform1i(glGetUniformLocation(fillShader->getShaderId(), "dsColor"), 0);
	glUniform1i(glGetUniformLocation(fillShader->getShaderId(), "dsDepth"), 1);
	glUniform1i(glGetUniformLocation(fillShader->getShaderId(), "dsDepthPrev"), 2);

	medianShader->recompile();
	medianShader->apply();
	glUniform1i(glGetUniformLocation(medianShader->getShaderId(), "dsColor"), 0);
	glUniform1i(glGetUniformLocation(medianShader->getShaderId(), "dsDepth"), 1);

	positionShader->recompile();
	positionShader->apply();
	glUniform1i(glGetUniformLocation(positionShader->getShaderId(), "dsColor"), 0);
	glUniform1i(glGetUniformLocation(positionShader->getShaderId(), "dsDepth"), 1);

	blurShader->recompile();
	blurShader->apply();
	glUniform1i(glGetUniformLocation(blurShader->getShaderId(), "dsColor"), 0);
	glUniform1i(glGetUniformLocation(blurShader->getShaderId(), "dsDepth"), 1);
	glUniform1i(glGetUniformLocation(blurShader->getShaderId(), "dsPosition"), 2);
	glUniform1i(glGetUniformLocation(blurShader->getShaderId(), "dsNormal"), 3);
}

void DSensor::update()
{
	if (!initOk || !isRendering) return;

	int changedIndex;
	openni::Status rc = openni::OpenNI::waitForAnyStream(streams, 2, &changedIndex);
	if (rc != openni::STATUS_OK)
	{
		printf("OpenNI Error: Wait for any stream failed\n");
		return;
	}

	openni::VideoFrameRef depthFrame;
	openni::VideoFrameRef colorFrame;
	switch (changedIndex)
	{
	case 0:
		depthStream.readFrame(&depthFrame);
		break;

	case 1:
		colorStream.readFrame(&colorFrame);
		break;

	default:
		printf("OpenNI Error: in wait\n");
	}

	// Copy color data from kinect into color buffer
	if (colorFrame.isValid())
	{
		const openni::RGB888Pixel* imageBuffer = (const openni::RGB888Pixel*)colorFrame.getData();
		memcpy(texColorMap, imageBuffer, colorFrame.getWidth() * colorFrame.getHeight() * 3 * sizeof(uint8_t));
		glBindTexture(GL_TEXTURE_2D, dsColorMap);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight, GL_RGB, GL_UNSIGNED_BYTE, texColorMap);
	}

	/*if (depthFrame.isValid())
	{
		const uint16_t *depthPixels = (const uint16_t*)depthFrame.getData();
		memcpy(texDepthMap, depthPixels, depthFrame.getWidth() * depthFrame.getHeight() * sizeof(uint16_t));
		glBindTexture(GL_TEXTURE_2D, gltexDepthMap);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight, GL_RED_INTEGER, GL_UNSIGNED_SHORT, texDepthMap);
	}*/

	// Map 11bit depth to 16 bit and copy into 16bit depth buffer
	if (depthFrame.isValid())
	{
		glBindTexture(GL_TEXTURE_2D, dsDepthMapPrev);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight, GL_RED, GL_UNSIGNED_SHORT, texDepthMap);

		memset(texDepthMap, 0, texWidth * texHeight * sizeof(uint16_t));

		const uint16_t* pDepthRow = (const uint16_t*)depthFrame.getData();
		uint16_t* pTexRow = texDepthMap + depthFrame.getCropOriginY() * texWidth;
		int rowSize = depthFrame.getStrideInBytes() / sizeof(uint16_t);

		float depthHist[maxDepth];
		calculateHistogram(depthHist, maxDepth, depthFrame);

		for (int y = 0; y < depthFrame.getHeight(); ++y)
		{
			const uint16_t* pDepth = pDepthRow;
			uint16_t* pTex = pTexRow + depthFrame.getCropOriginX();

			for (int x = 0; x < depthFrame.getWidth(); ++x, ++pDepth, ++pTex)
			{
				if (*pDepth != 0)
				{
					int nHistValue = (int)depthHist[*pDepth];
					*pTex = nHistValue;
				}
			}

			pDepthRow += rowSize;
			pTexRow += texWidth;
		}

		glBindTexture(GL_TEXTURE_2D, dsDepthMap);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight, GL_RED, GL_UNSIGNED_SHORT, texDepthMap);
	}

	if (depthFrame.isValid())
	{
		// 1st Pass, filp kinect y textures, fill holes
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, texWidth, texHeight);
		glClear(GL_COLOR_BUFFER_BIT);

		fillShader->apply();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, dsColorMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, dsDepthMap);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, dsDepthMapPrev);

		quad->draw();

		// Median filter pass
		glBindFramebuffer(GL_FRAMEBUFFER, fbo2);
		glViewport(0, 0, texWidth, texHeight);
		glClear(GL_COLOR_BUFFER_BIT);

		medianShader->apply();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, outColorMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, outDepthMap);

		quad->draw();

		// Generate position and normal pass
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, texWidth, texHeight);
		glClear(GL_COLOR_BUFFER_BIT);

		positionShader->apply();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, outColorMap2);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, outDepthMap2);

		quad->draw();

		// Blur pass
		glBindFramebuffer(GL_FRAMEBUFFER, fbo2);
		glViewport(0, 0, texWidth, texHeight);
		glClear(GL_COLOR_BUFFER_BIT);

		blurShader->apply();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, outColorMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, outDepthMap);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, outPositionMap);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, outNormalMap);

		quad->draw();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

GLuint DSensor::getColorMapId() const
{
	return outColorMap2;
}

GLuint DSensor::getDepthMapId() const
{
	return outDepthMap2;
}

GLuint DSensor::getPositionMapId() const
{
	return outPositionMap2;
}

GLuint DSensor::getNormalMapId() const
{
	return outNormalMap2;
}

glm::mat4 DSensor::getMatProjection() const
{
	return matProjection;
}

glm::mat4 DSensor::getMatProjectionInverse() const
{
	return matProjectionInverse;
}

void DSensor::toggleRendering()
{
	isRendering = !isRendering;
}

GLuint DSensor::minNumChunks(GLuint dataSize, GLuint chunkSize)
{
	return (dataSize - 1) / chunkSize + 1;
}

GLuint DSensor::minChunkSize(GLuint dataSize, GLuint chunkSize)
{
	return minNumChunks(dataSize, chunkSize) * chunkSize;
}

// Fit kinect data to 16bit buffer
void DSensor::calculateHistogram(float* pHistogram, int histogramSize, const openni::VideoFrameRef& frame)
{
	const uint16_t* pDepth = (const uint16_t*)frame.getData();
	memset(pHistogram, 0, histogramSize * sizeof(float));
	int restOfRow = frame.getStrideInBytes() / sizeof(uint16_t) - frame.getWidth();
	int height = frame.getHeight();
	int width = frame.getWidth();

	unsigned int nNumberOfPoints = 0;
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x, ++pDepth)
		{
			if (*pDepth != 0)
			{
				pHistogram[*pDepth]++;
				nNumberOfPoints++;
			}
		}
		pDepth += restOfRow;
	}
	//std::cout << restOfRow << "\n";

	for (int nIndex = 1; nIndex < histogramSize; nIndex++)
	{
		pHistogram[nIndex] += pHistogram[nIndex - 1];
	}

	if (nNumberOfPoints)
	{
		for (int nIndex = 1; nIndex < histogramSize; nIndex++)
		{
			pHistogram[nIndex] = (65536 * (1.0f - (pHistogram[nIndex] / nNumberOfPoints)));
		}
	}
}