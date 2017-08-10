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

void DSensor::initializeShaders()
{
	temporalMedianShader->apply();
	glUniform1i(glGetUniformLocation(temporalMedianShader->getShaderId(), "dsColor"), 0);
	glUniform1i(glGetUniformLocation(temporalMedianShader->getShaderId(), "dsDepth"), 1);
	glUniform1i(glGetUniformLocation(temporalMedianShader->getShaderId(), "kernelRadius"), tmfKernelRadius);
	glUniform1i(glGetUniformLocation(temporalMedianShader->getShaderId(), "frameLayers"), tmfFrameLayers);

	medianShader->apply();
	glUniform1i(glGetUniformLocation(medianShader->getShaderId(), "dsColor"), 0);
	glUniform1i(glGetUniformLocation(medianShader->getShaderId(), "dsDepth"), 1);
	glUniform1i(glGetUniformLocation(medianShader->getShaderId(), "kernelRadius"), fillKernelRadius);

	setBlurKernelRadius(blurKernelRadius);
	blurShader->apply();
	glUniform1i(glGetUniformLocation(blurShader->getShaderId(), "dsColor"), 0);
	glUniform1i(glGetUniformLocation(blurShader->getShaderId(), "dsDepth"), 1);
	glUniform1i(glGetUniformLocation(blurShader->getShaderId(), "kernelRadius"), blurKernelRadius);
	glUniform1fv(glGetUniformLocation(blurShader->getShaderId(), "kernel"), blurKernelRadius * 2 + 1, &blurKernel[0]);

	positionShader->apply();
	glUniform1i(glGetUniformLocation(positionShader->getShaderId(), "dsColor"), 0);
	glUniform1i(glGetUniformLocation(positionShader->getShaderId(), "dsDepth"), 1);
}

void DSensor::recompileShaders()
{
	if (hasError) return;

	temporalMedianShader->recompile();
	medianShader->recompile();
	blurShader->recompile();
	positionShader->recompile();
	
	initializeShaders();
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

	//bufferWidth = windowWidth;
	//bufferHeight = windowHeight;
	bufferWidth = texWidth;
	bufferHeight = texHeight;

	device.setImageRegistrationMode(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR);

	if (!hasError)
	{
		printf("\tOK\n");
		initOk = true;
	}

	glGenTextures(1, &dsColorMap);
	glBindTexture(GL_TEXTURE_2D, dsColorMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, texWidth, texHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &dsDepthMap);
	glBindTexture(GL_TEXTURE_2D_ARRAY, dsDepthMap);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R16, texWidth, texHeight, tmfMaxFrameLayers);
	//glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R16, texWidth, texHeight, tmfFrameLayers, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	

	quad = new Quad();
	temporalMedianShader = new Shader("dsTemporalMedian");
	medianShader = new Shader("dsMedian");
	positionShader = new Shader("dsPosition");
	blurShader = new Shader("dsBlur");
	initializeShaders();
	
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGenTextures(1, &outColorMap);
	glBindTexture(GL_TEXTURE_2D, outColorMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outColorMap, 0);

	glGenTextures(1, &outDepthMap);
	glBindTexture(GL_TEXTURE_2D, outDepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, bufferWidth, bufferHeight, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, outDepthMap, 0);

	glGenTextures(1, &outPositionMap);
	glBindTexture(GL_TEXTURE_2D, outPositionMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, outPositionMap, 0);

	glGenTextures(1, &outNormalMap);
	glBindTexture(GL_TEXTURE_2D, outNormalMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, outNormalMap, 0);

	const GLenum attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);

	glGenFramebuffers(1, &fbo2);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo2);

	glGenTextures(1, &outColorMap2);
	glBindTexture(GL_TEXTURE_2D, outColorMap2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outColorMap2, 0);

	glGenTextures(1, &outDepthMap2);
	glBindTexture(GL_TEXTURE_2D, outDepthMap2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, bufferWidth, bufferHeight, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, outDepthMap2, 0);

	glGenTextures(1, &outPositionMap2);
	glBindTexture(GL_TEXTURE_2D, outPositionMap2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, outPositionMap2, 0);

	glGenTextures(1, &outNormalMap2);
	glBindTexture(GL_TEXTURE_2D, outNormalMap2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, outNormalMap2, 0);

	glDrawBuffers(4, attachments);
}

void DSensor::update()
{
	if (!initOk || !isRendering) return;

	int changedIndex;
	openni::Status rc = openni::OpenNI::waitForAnyStream(streams, 2, &changedIndex, 0);
	if (rc != openni::STATUS_OK)
	{
		//printf("OpenNI Error: Wait for any stream failed\n");
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
		// Store previous depth frames
		if (dsDepthMapLayerCounter == tmfFrameLayers) dsDepthMapLayerCounter = 1;
		glBindTexture(GL_TEXTURE_2D_ARRAY, dsDepthMap);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, dsDepthMapLayerCounter++, texWidth, texHeight, 1, GL_RED, GL_UNSIGNED_SHORT, texDepthMap);

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

		glBindTexture(GL_TEXTURE_2D_ARRAY, dsDepthMap);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, texWidth, texHeight, 1, GL_RED, GL_UNSIGNED_SHORT, texDepthMap);
	}

	if (depthFrame.isValid())
	{
		// Filp kinect y textures
		// Temporal median filter pass
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, bufferWidth, bufferHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		temporalMedianShader->apply();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, dsColorMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D_ARRAY, dsDepthMap);

		quad->draw();

		// Fill holes passes
		GLuint currFbo = fbo2;
		GLuint currColorMap = outColorMap;
		GLuint currDepthMap = outDepthMap;
		for (int i = 0; i < fillPasses; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, currFbo);
			glViewport(0, 0, bufferWidth, bufferHeight);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			medianShader->apply();

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, currColorMap);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, currDepthMap);

			quad->draw();

			currFbo = currFbo == fbo2 ? fbo : fbo2;
			currColorMap = currColorMap == outColorMap ? outColorMap2 : outColorMap;
			currDepthMap = currDepthMap == outDepthMap ? outDepthMap2 : outDepthMap;
		}

		// Gaussian filter pass (seperated passes)
		currFbo = fbo2;
		currColorMap = outColorMap;
		currDepthMap = outDepthMap;
		int isVertical = 0;
		for (int i = 0; i < 2; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, currFbo);
			glViewport(0, 0, bufferWidth, bufferHeight);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			blurShader->apply();
			glUniform1i(glGetUniformLocation(blurShader->getShaderId(), "isVertical"), isVertical);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, currColorMap);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, currDepthMap);

			quad->draw();

			isVertical++;
			currFbo = currFbo == fbo2 ? fbo : fbo2;
			currColorMap = currColorMap == outColorMap ? outColorMap2 : outColorMap;
			currDepthMap = currDepthMap == outDepthMap ? outDepthMap2 : outDepthMap;
		}

		// Generate position and normal pass
		glBindFramebuffer(GL_FRAMEBUFFER, fbo2);
		glViewport(0, 0, bufferWidth, bufferHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		positionShader->apply();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, outColorMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, outDepthMap);

		quad->draw();

		

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

void DSensor::updateThread(std::atomic<bool>& isRunning, unsigned int updateInterval)
{
	const auto wait_duration = std::chrono::milliseconds(updateInterval);
	while (isRunning)
	{
		//update();
		std::cout << "test" << std::endl;
		std::this_thread::sleep_for(wait_duration);
	}
}

void DSensor::launchUpdateThread()
{
	std::atomic<bool> running{ true };
	const unsigned int update_interval = 50;
	std::thread openniThread(&DSensor::updateThread, this, std::ref(running), update_interval);
	openniThread.detach();
}

void DSensor::toggleRendering()
{
	isRendering = !isRendering;
}

void DSensor::getCustomPixelInfo(int sampleRadius, std::vector<glm::vec3>*& outPositions, std::vector<glm::vec3>*& outNormals)
{
	customPositions.clear();
	customNormals.clear();

	glBindFramebuffer(GL_FRAMEBUFFER, fbo2);

	// Get positions
	glReadBuffer(GL_COLOR_ATTACHMENT2);
	glm::vec2 sampleDiff = glm::vec2(bufferWidth, bufferHeight) / (sampleRadius * 2);

	for (int i = -sampleRadius; i <= sampleRadius; i++)
	{
		for (int j = -sampleRadius; j <= sampleRadius; j++)
		{
			glm::vec3 out;
			glReadPixels((int)((sampleDiff.x * i) / 1.5 + bufferWidth / 2), (int)((sampleDiff.y * j) / 1.5 + bufferHeight / 2), 1, 1, GL_RGB, GL_FLOAT, &out);
			customPositions.push_back(out);
		}
	}

	// Get normals
	glReadBuffer(GL_COLOR_ATTACHMENT3);

	for (int i = -sampleRadius; i <= sampleRadius; i++)
	{
		for (int j = -sampleRadius; j <= sampleRadius; j++)
		{
			glm::vec3 out;
			glReadPixels((int)((sampleDiff.x * i) / 1.5 + bufferWidth / 2), (int)((sampleDiff.y * j) / 1.5 + bufferHeight / 2), 1, 1, GL_RGB, GL_FLOAT, &out);
			customNormals.push_back(out);
		}
	}

	outPositions = &customPositions;
	outNormals = &customNormals;
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

int DSensor::getTMFKernelRadius() const
{
	return tmfKernelRadius;
}

int DSensor::getTMFFrameLayers() const
{
	return tmfFrameLayers;
}

int DSensor::getFillKernelRaidus() const
{
	return fillKernelRadius;
}

int DSensor::getFillPasses() const
{
	return fillPasses;
}

int DSensor::getBlurKernelRadius() const
{
	return blurKernelRadius;
}

float DSensor::getBlurSigma() const
{
	return blurSigma;
}

float DSensor::getBlurBSigma() const
{
	return blurBSigma;
}

void DSensor::setTMFKernelRadius(int value)
{
	tmfKernelRadius = value;

	temporalMedianShader->apply();
	glUniform1i(glGetUniformLocation(temporalMedianShader->getShaderId(), "kernelRadius"), tmfKernelRadius);
}

void DSensor::setTMFFrameLayers(int value)
{
	tmfFrameLayers = min(value, tmfMaxFrameLayers);
	dsDepthMapLayerCounter = 1;

	temporalMedianShader->apply();
	glUniform1i(glGetUniformLocation(temporalMedianShader->getShaderId(), "frameLayers"), tmfFrameLayers);
}

void DSensor::setFillKernelRaidus(int value)
{
	fillKernelRadius = value;

	medianShader->apply();
	glUniform1i(glGetUniformLocation(medianShader->getShaderId(), "kernelRadius"), fillKernelRadius);
}

void DSensor::setFillPasses(int value)
{
	if (value % 2 == 1) fillPasses = value;
}

float DSensor::normpdf(float x, float s)
{
	return 1 / (s * s * 2 * 3.14159265f) * exp(-x * x / (2 * s * s)) / s;
}

void DSensor::computeBlurKernel()
{
	blurKernel = std::vector<float>(blurKernelRadius * 2 + 1);

	for (int i = 0; i <= blurKernelRadius; i++)
	{
		float value = normpdf((float)i, blurSigma);
		blurKernel.at(blurKernelRadius + i) = blurKernel.at(blurKernelRadius - i) = value;
	}

	blurShader->apply();
	glUniform1i(glGetUniformLocation(blurShader->getShaderId(), "kernelRadius"), blurKernelRadius);
	glUniform1fv(glGetUniformLocation(blurShader->getShaderId(), "kernel"), blurKernelRadius * 2 + 1, &blurKernel[0]);
}

void DSensor::setBlurKernelRadius(int value)
{
	blurKernelRadius = value;
	computeBlurKernel();
}

void DSensor::setBlurSigma(float value)
{
	blurSigma = value;
	computeBlurKernel();
}

void DSensor::setBlurBSigma(float value)
{
	blurBSigma = value;
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