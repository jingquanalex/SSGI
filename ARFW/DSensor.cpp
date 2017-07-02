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
	float w = (float)windowWidth / videoWidth * fovX;
	float h = (float)windowHeight / videoHeight * fovY;
	matProjection = glm::perspective(glm::radians(fovY), w / h, 0.1f, 1000.0f);
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

	glGenTextures(1, &gltexColorMap);
	glBindTexture(GL_TEXTURE_2D, gltexColorMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &gltexDepthMap);
	glBindTexture(GL_TEXTURE_2D, gltexDepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, texWidth, texHeight, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void DSensor::render()
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
		glBindTexture(GL_TEXTURE_2D, gltexColorMap);
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
		memset(texDepthMap, 0, texWidth * texHeight * sizeof(uint16_t));

		const openni::DepthPixel* pDepthRow = (const openni::DepthPixel*)depthFrame.getData();
		uint16_t* pTexRow = texDepthMap + depthFrame.getCropOriginY() * texWidth;
		int rowSize = depthFrame.getStrideInBytes() / sizeof(openni::DepthPixel);

		float depthHist[maxDepth];
		calculateHistogram(depthHist, maxDepth, depthFrame);

		for (int y = 0; y < depthFrame.getHeight(); ++y)
		{
			const openni::DepthPixel* pDepth = pDepthRow;
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

		glBindTexture(GL_TEXTURE_2D, gltexDepthMap);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight, GL_RED, GL_UNSIGNED_SHORT, texDepthMap);
	}
}

GLuint DSensor::getColorMapId() const
{
	return gltexColorMap;
}

GLuint DSensor::getDepthMapId() const
{
	return gltexDepthMap;
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

// Fit kinect 11bit depth to 16bit buffer
void DSensor::calculateHistogram(float* pHistogram, int histogramSize, const openni::VideoFrameRef& frame)
{
	const openni::DepthPixel* pDepth = (const openni::DepthPixel*)frame.getData();
	memset(pHistogram, 0, histogramSize * sizeof(float));
	int restOfRow = frame.getStrideInBytes() / sizeof(openni::DepthPixel) - frame.getWidth();
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
	for (int nIndex = 1; nIndex<histogramSize; nIndex++)
	{
		pHistogram[nIndex] += pHistogram[nIndex - 1];
	}
	if (nNumberOfPoints)
	{
		for (int nIndex = 1; nIndex<histogramSize; nIndex++)
		{
			pHistogram[nIndex] = (65536 * (1.0f - (pHistogram[nIndex] / nNumberOfPoints)));
		}
	}
}