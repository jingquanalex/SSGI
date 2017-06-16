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

void DSensor::initialize(GLuint texSize)
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

	// Texture map init
	// Assume texture is square, color and depth size are same
	this->texSize = texSize;
	texWidth = minChunkSize(videoWidth, texSize);
	texHeight = minChunkSize(videoHeight, texSize);
	texColorMap = new openni::RGB888Pixel[texWidth * texHeight];
	texDepthMap = new openni::RGB888Pixel[texWidth * texHeight];

	device.setImageRegistrationMode(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR);

	if (!hasError)
	{
		printf("\tOK\n");
		initOk = true;
	}
}

void DSensor::render()
{
	if (!initOk) return;

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

	// check if we need to draw image frame to texture
	if (colorFrame.isValid())
	{
		memset(texColorMap, 0, texWidth * texHeight * sizeof(openni::RGB888Pixel));

		const openni::RGB888Pixel* pImageRow = (const openni::RGB888Pixel*)colorFrame.getData();
		openni::RGB888Pixel* pTexRow = texColorMap + colorFrame.getCropOriginY() * texWidth;
		int rowSize = colorFrame.getStrideInBytes() / sizeof(openni::RGB888Pixel);

		for (int y = 0; y < colorFrame.getHeight(); ++y)
		{
			const openni::RGB888Pixel* pImage = pImageRow;
			openni::RGB888Pixel* pTex = pTexRow + colorFrame.getCropOriginX();

			for (int x = 0; x < colorFrame.getWidth(); ++x, ++pImage, ++pTex)
			{
				*pTex = *pImage;
			}

			pImageRow += rowSize;
			pTexRow += texWidth;
		}
	}

	// check if we need to draw depth frame to texture
	if (depthFrame.isValid())
	{
		memset(texDepthMap, 0, texWidth * texHeight * sizeof(openni::RGB888Pixel));

		const openni::DepthPixel* pDepthRow = (const openni::DepthPixel*)depthFrame.getData();
		openni::RGB888Pixel* pTexRow = texDepthMap + depthFrame.getCropOriginY() * texWidth;
		int rowSize = depthFrame.getStrideInBytes() / sizeof(openni::DepthPixel);

		float depthHist[maxDepth];
		calculateHistogram(depthHist, maxDepth, depthFrame);

		for (int y = 0; y < depthFrame.getHeight(); ++y)
		{
			const openni::DepthPixel* pDepth = pDepthRow;
			openni::RGB888Pixel* pTex = pTexRow + depthFrame.getCropOriginX();

			for (int x = 0; x < depthFrame.getWidth(); ++x, ++pDepth, ++pTex)
			{
				if (*pDepth != 0)
				{
					int nHistValue = (int)depthHist[*pDepth];
					pTex->r = nHistValue;
					pTex->g = nHistValue;
					pTex->b = 0;
				}
			}

			pDepthRow += rowSize;
			pTexRow += texWidth;
		}
	}

	glGenTextures(1, &gltexColorMap);
	glBindTexture(GL_TEXTURE_2D, gltexColorMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, texColorMap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glGenTextures(1, &gltexDepthMap);
	glBindTexture(GL_TEXTURE_2D, gltexDepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, texDepthMap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint DSensor::getColorMapId() const
{
	return gltexColorMap;
}

GLuint DSensor::getDepthMapId() const
{
	return gltexDepthMap;
}

GLuint DSensor::minNumChunks(GLuint dataSize, GLuint chunkSize)
{
	return (dataSize - 1) / chunkSize + 1;
}

GLuint DSensor::minChunkSize(GLuint dataSize, GLuint chunkSize)
{
	return minNumChunks(dataSize, chunkSize) * chunkSize;
}

void DSensor::calculateHistogram(float* pHistogram, int histogramSize, const openni::VideoFrameRef& frame)
{
	const openni::DepthPixel* pDepth = (const openni::DepthPixel*)frame.getData();
	// Calculate the accumulative histogram (the yellow display...)
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
			pHistogram[nIndex] = (256 * (1.0f - (pHistogram[nIndex] / nNumberOfPoints)));
		}
	}
}