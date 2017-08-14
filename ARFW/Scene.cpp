#include "Scene.h"

using namespace std;
using namespace glm;

extern string g_ExePath;
extern int g_windowWidth;
extern int g_windowHeight;

Scene::Scene()
{
}

Scene::~Scene()
{
	if (sensor != nullptr) delete sensor;
	if (camera != nullptr) delete camera;
	if (gPassShader != nullptr) delete gPassShader;
	if (lightingPassShader != nullptr) delete lightingPassShader;
	if (quad != nullptr) delete quad;
	if (dragon != nullptr) delete dragon;
}

void Scene::recompileShaders()
{
	pbr->recompileShaders();
	sensor->recompileShaders();
	gPassShader->recompile();
	gComposePassShader->recompile();
	ssao->recompileShaders();
	lightingPassShader->recompile();
	ssr->recompileShaders();
	compositeShader->recompile();
	outputShader->recompile();
	pointCloud->recompileShader();
	initializeShaders();
}

void Scene::initializeShaders()
{
	gPassShader->apply();
	glUniform1i(glGetUniformLocation(gPassShader->getShaderId(), "diffuse1"), 0);
	glUniform1i(glGetUniformLocation(gPassShader->getShaderId(), "normal1"), 1);
	glUniform1i(glGetUniformLocation(gPassShader->getShaderId(), "specular1"), 2);

	gComposePassShader->apply();
	glUniform1i(glGetUniformLocation(gComposePassShader->getShaderId(), "inPosition"), 0);
	glUniform1i(glGetUniformLocation(gComposePassShader->getShaderId(), "inNormal"), 1);
	glUniform1i(glGetUniformLocation(gComposePassShader->getShaderId(), "inColor"), 2);
	glUniform1i(glGetUniformLocation(gComposePassShader->getShaderId(), "dsPosition"), 3);
	glUniform1i(glGetUniformLocation(gComposePassShader->getShaderId(), "dsNormal"), 4);
	glUniform1i(glGetUniformLocation(gComposePassShader->getShaderId(), "dsColor"), 5);

	lightingPassShader->apply();
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "gPosition"), 0);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "gNormal"), 1);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "gColor"), 2);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "aoMap"), 3);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "irradianceMap"), 4);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "prefilterMap"), 5);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "brdfLUT"), 6);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "reflectionMap"), 7);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "reflectanceMap"), 8);

	compositeShader->apply();
	glUniform1i(glGetUniformLocation(compositeShader->getShaderId(), "fullScene"), 0);
	glUniform1i(glGetUniformLocation(compositeShader->getShaderId(), "backScene"), 1);
	glUniform1i(glGetUniformLocation(compositeShader->getShaderId(), "dsColor"), 2);

	outputShader->apply();
	glUniform1i(glGetUniformLocation(outputShader->getShaderId(), "displayMode"), renderMode);
	glUniform1i(glGetUniformLocation(outputShader->getShaderId(), "inColor"), 0);
	glUniform1i(glGetUniformLocation(outputShader->getShaderId(), "gPosition"), 1);
	glUniform1i(glGetUniformLocation(outputShader->getShaderId(), "gNormal"), 2);
	glUniform1i(glGetUniformLocation(outputShader->getShaderId(), "gColor"), 3);
	glUniform1i(glGetUniformLocation(outputShader->getShaderId(), "aoMap"), 4);
	glUniform1i(glGetUniformLocation(outputShader->getShaderId(), "dsColor"), 5);
	glUniform1i(glGetUniformLocation(outputShader->getShaderId(), "dsDepth"), 6);
	glUniform1i(glGetUniformLocation(outputShader->getShaderId(), "tex1"), 7);
}

void Scene::initialize(nanogui::Screen* guiScreen)
{
	// Fix to preset resolution for now
	bufferWidth = g_windowWidth;
	bufferHeight = g_windowHeight;

	// Load framework objects
	timerRunOnceOnStart = new Timer(2.0f, 2.0f);
	camera = new CameraFPS(bufferWidth, bufferHeight);
	camera->setActive(true);
	camera->setMoveSpeed(0.5);
	camera->setPosition(vec3(0, 0, 0));
	camera->setDirection(vec3(0, 0, -1));
	gPassShader = new Shader("gPass");
	lightingPassShader = new Shader("lightingPass");
	gComposePassShader = new Shader("gComposePass");

	ssr = new SSReflection(bufferWidth, bufferHeight);
	ssao = new SSAO(bufferWidth, bufferHeight);
	quad = new Quad();
	plane = new Object();
	plane->setGPassShaderId(gPassShader->getShaderId());
	plane->setPosition(vec3(0.0f, -0.2f, -0.3f));
	plane->setScale(vec3(0.5f, 0.2f, 0.5f));
	plane->load("cube/cube.obj");
	knob = new Object();
	knob->setGPassShaderId(gPassShader->getShaderId());
	knob->setPosition(vec3(0.0f, -0.05f, 0.3f));
	knob->setScale(vec3(0.05f));
	knob->load("knob/mitsuba-sphere.obj");
	dragon = new Object();
	dragon->setGPassShaderId(gPassShader->getShaderId());
	//dragon->setBoundingBoxVisible(true);
	//dragon->setScale(vec3(0.05f));
	dragon->setScale(vec3(0.25f));
	dragon->setPosition(vec3(0, 0, -0.5));
	//dragon->setRotationEulerAngle(vec3(0, 90, 0));
	dragon->setRotationByAxisAngle(vec3(0, 1, 0), 90);
	dragon->load("dragon.obj");
	//dragon->load("sibenik/sibenik.obj");
	//dragon->load("dragon/dragon.obj");
	texWhite = Image::loadTexture(g_ExePath + "../../media/white.png");
	texRed = Image::loadTexture(g_ExePath + "../../media/red.png");
	texGreen = Image::loadTexture(g_ExePath + "../../media/green.png");
	texBlue = Image::loadTexture(g_ExePath + "../../media/blue.png");

	// Initialize depth sensor
	sensor = new DSensor();
	sensor->initialize(bufferWidth, bufferHeight);

	randomFloats = uniform_real_distribution<float>(0.0f, 1.0f);

	pointCloud = new PointCloud(sensor->getColorMapId(), sensor->getDepthMapId());

	// Initialize GUI
	this->guiScreen = guiScreen;
	gui = new nanogui::FormHelper(guiScreen);
	nanoguiWindow = gui->addWindow(Eigen::Vector2i(10, 10), "ARFW");

	gui->addGroup("Misc");
	gui->addButton("Recompile Shaders", [&]()
	{
		recompileShaders();
	});

	gui->addButton("Reset Camera", [&]()
	{
		camera->setPosition(vec3(0, 0, 0));
		//camera->setDirection(vec3(0, 0, -1));
		camera->setTargetPoint(camera->getPosition() + vec3(0, 0, -1));
	});

	gui->addVariable("dragonNumRadius", dragonNumRadius);
	gui->addButton("Spawn Dragons", [&]()
	{
		spawnDragons(dragonNumRadius);
	});
	gui->addButton("Recompute envmap", [&]()
	{
		pbr->recomputeEnvMaps(dsColor);
	});

	gui->addGroup("Light/Material");
	gui->addVariable("Position X", lightPosition.x);
	gui->addVariable("Position Y", lightPosition.y);
	gui->addVariable("Position Z", lightPosition.z);
	gui->addVariable("Color", lightColor);
	gui->addVariable("Roughness", roughness)->setSpinnable(true);
	gui->addVariable("Metallic", metallic)->setSpinnable(true);

	gui->addGroup("Kinect depth filters");
	gui->addGroup("Temporal median filter");
	gui->addVariable<int>("kernelRadius",
		[&](const int &value) { sensor->setTMFKernelRadius(value); },
		[&]() { return sensor->getTMFKernelRadius(); });
	gui->addVariable<int>("frameLayers (max 10)",
		[&](const int &value) { sensor->setTMFFrameLayers(value); },
		[&]() { return sensor->getTMFFrameLayers(); });

	gui->addGroup("Fill holes (median)");
	gui->addVariable<int>("kernelRadius",
		[&](const int &value) { sensor->setFillKernelRaidus(value); },
		[&]() { return sensor->getFillKernelRaidus(); });
	gui->addVariable<int>("passes (odd value only)",
		[&](const int &value) { sensor->setFillPasses(value); },
		[&]() { return sensor->getFillPasses(); });

	gui->addGroup("Bilateral filter");
	gui->addVariable<int>("kernelRadius",
		[&](const int &value) { sensor->setBlurKernelRadius(value); },
		[&]() { return sensor->getBlurKernelRadius(); });
	gui->addVariable<float>("sigma",
		[&](const float &value) { sensor->setBlurSigma(value); },
		[&]() { return sensor->getBlurSigma(); });
	gui->addVariable<float>("sigma (depth)",
		[&](const float &value) { sensor->setBlurBSigma(value); },
		[&]() { return sensor->getBlurBSigma(); });

	gui->addGroup("SSAO");
	gui->addVariable<int>("kernelSize (SSAO)",
		[&](const int &value) { ssao->setKernelSize(value); },
		[&]() { return ssao->getKernelSize(); });
	gui->addVariable<float>("kernelRadius",
		[&](const float &value) { ssao->setKernelRadius(value); },
		[&]() { return ssao->getKernelRadius(); });
	gui->addVariable<int>("samples (AlchemyAO)",
		[&](const int &value) { ssao->setSamples(value); },
		[&]() { return ssao->getSamples(); });
	gui->addVariable<float>("bias",
		[&](const float &value) { ssao->setBias(value); },
		[&]() { return ssao->getBias(); });
	gui->addVariable<float>("intensity",
		[&](const float &value) { ssao->setIntensity(value); },
		[&]() { return ssao->getIntensity(); });
	gui->addVariable<float>("power",
		[&](const float &value) { ssao->setPower(value); },
		[&]() { return ssao->getPower(); });
	gui->addVariable<int>("filterKernelRadius",
		[&](const int &value) { ssao->setBlurKernelRadius(value); },
		[&]() { return ssao->getBlurKernelRadius(); });
	gui->addVariable<float>("filterSigma",
		[&](const float &value) { ssao->setBlurSigma(value); },
		[&]() { return ssao->getBlurSigma(); });
	gui->addVariable<float>("filterSigma (depth)",
		[&](const float &value) { ssao->setBlurZSigma(value); },
		[&]() { return ssao->getBlurZSigma(); });
	gui->addVariable<float>("filterSigma (normals)",
		[&](const float &value) { ssao->setBlurNSigma(value); },
		[&]() { return ssao->getBlurNSigma(); });

	Eigen::Vector2i windowSize = nanoguiWindow->size();
	nanoguiWindow2 = gui->addWindow(Eigen::Vector2i(250, 10), "More options");

	gui->addGroup("Screen Space Raytracing");
	gui->addVariable<float>("maxSteps",
		[&](const float &value) { ssr->setMaxSteps(value); },
		[&]() { return ssr->getMaxSteps(); });
	gui->addVariable<float>("binarySearchSteps",
		[&](const float &value) { ssr->setBinarySearchSteps(value); },
		[&]() { return ssr->getBinarySearchSteps(); });
	gui->addVariable<float>("maxRayTraceDistance",
		[&](const float &value) { ssr->setMaxRayTraceDistance(value); },
		[&]() { return ssr->getMaxRayTraceDistance(); });
	gui->addVariable<float>("nearPlaneZ",
		[&](const float &value) { ssr->setNearPlaneZ(value); },
		[&]() { return ssr->getNearPlaneZ(); });
	gui->addVariable<float>("rayZThickness",
		[&](const float &value) { ssr->setRayZThickness(value); },
		[&]() { return ssr->getRayZThickness(); });
	gui->addVariable<float>("stride",
		[&](const float &value) { ssr->setStride(value); },
		[&]() { return ssr->getStride(); });
	gui->addVariable<float>("strideZCutoff",
		[&](const float &value) { ssr->setStrideZCutoff(value); },
		[&]() { return ssr->getStrideZCutoff(); });
	gui->addVariable<float>("jitterFactor",
		[&](const float &value) { ssr->setJitterFactor(value); },
		[&]() { return ssr->getJitterFactor(); });

	gui->addGroup("Alpha (fade ray sample)");
	gui->addVariable<float>("screenEdgeFadeStart",
		[&](const float &value) { ssr->setScreenEdgeFadeStart(value); },
		[&]() { return ssr->setScreenEdgeFadeStart(); });
	gui->addVariable<float>("cameraFadeStart",
		[&](const float &value) { ssr->setCameraFadeStart(value); },
		[&]() { return ssr->setCameraFadeStart(); });
	gui->addVariable<float>("cameraFadeLength",
		[&](const float &value) { ssr->setCameraFadeLength(value); },
		[&]() { return ssr->setCameraFadeLength(); });

	gui->addGroup("Light convolution");
	gui->addVariable<int>("maxMipLevels",
		[&](const int &value) { ssr->setMaxMipLevels(value); },
		[&]() { return ssr->getMaxMipLevels(); });
	gui->addVariable<float>("mipBasePower",
		[&](const float &value) { ssr->setMipBasePower(value); },
		[&]() { return ssr->getMipBasePower(); });
	gui->addVariable<float>("bsigma",
		[&](const float &value) { ssr->setGaussianBSigma(value); },
		[&]() { return ssr->getGaussianBSigma(); });

	gui->addGroup("Blurry reflections");
	gui->addVariable<float>("sharpness",
		[&](const float &value) { ssr->setSharpness(value); },
		[&]() { return ssr->getSharpness(); });
	gui->addVariable<float>("sharpnessPower",
		[&](const float &value) { ssr->setSharpnessPower(value); },
		[&]() { return ssr->getSharpnessPower(); });
	gui->addVariable<float>("mipLevel",
		[&](const float &value) { ssr->setConeTraceMipLevel(value); },
		[&]() { return ssr->getConeTraceMipLevel(); });

	gui->addGroup("Tonemapping");
	gui->addVariable<float>("exposure",
		[&](const float &value) { setExposure(value); },
		[&]() { return getExposure(); });
	
	guiScreen->setVisible(true);
	guiScreen->performLayout();

	// Initialize CamMat uniform buffer
	glGenBuffers(1, &uniform_CamMat);
	glBindBuffer(GL_UNIFORM_BUFFER, uniform_CamMat);
	glBufferData(GL_UNIFORM_BUFFER, 6 * sizeof(mat4), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 9, uniform_CamMat);

	// Set matrices to identity
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), value_ptr(mat4(1)));
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4), sizeof(mat4), value_ptr(mat4(1)));
	glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(mat4), sizeof(mat4), value_ptr(mat4(1)));
	glBufferSubData(GL_UNIFORM_BUFFER, 3 * sizeof(mat4), sizeof(mat4), value_ptr(mat4(1)));
	glBufferSubData(GL_UNIFORM_BUFFER, 4 * sizeof(mat4), sizeof(mat4), value_ptr(mat4(1)));
	glBufferSubData(GL_UNIFORM_BUFFER, 5 * sizeof(mat4), sizeof(mat4), value_ptr(mat4(1)));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// Precompute PBR environment maps
	pbr = new PBR(bufferWidth, bufferHeight);
	pbr->computeEnvMaps();

	// Deferred rendering buffer
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

	glGenTextures(1, &gColor);
	glBindTexture(GL_TEXTURE_2D, gColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bufferWidth, bufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gColor, 0);

	glGenTextures(1, &gDepth);
	glBindTexture(GL_TEXTURE_2D, gDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, bufferWidth, bufferHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gDepth, 0);

	const GLenum attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	// Create buffers for gComposeBuffer pass
	glGenFramebuffers(1, &gComposeBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gComposeBuffer);

	glGenTextures(1, &gComposedPosition);
	glBindTexture(GL_TEXTURE_2D, gComposedPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gComposedPosition, 0);

	glGenTextures(1, &gComposedNormal);
	glBindTexture(GL_TEXTURE_2D, gComposedNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gComposedNormal, 0);

	glGenTextures(1, &gComposedColor);
	glBindTexture(GL_TEXTURE_2D, gComposedColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bufferWidth, bufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gComposedColor, 0);

	// Output kinect ds textures again to make sure texture sampling are even
	// Else differential rendering produces artifacts if lowres ds texture are directly used
	glGenTextures(1, &dsPosition);
	glBindTexture(GL_TEXTURE_2D, dsPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, dsPosition, 0);

	glGenTextures(1, &dsNormal);
	glBindTexture(GL_TEXTURE_2D, dsNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, dsNormal, 0);

	glGenTextures(1, &dsColor);
	glBindTexture(GL_TEXTURE_2D, dsColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bufferWidth, bufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, GL_TEXTURE_2D, dsColor, 0);

	const GLenum gComposeAttachments[6] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2,
		GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5 };
	glDrawBuffers(6, gComposeAttachments);

	// Capture final outputs for differential rendering
	compositeShader = new Shader("composite");
	outputShader = new Shader("output");

	glGenFramebuffers(1, &captureFBO);

	glGenTextures(1, &cLightingFull);
	glBindTexture(GL_TEXTURE_2D, cLightingFull);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bufferWidth, bufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &cLightingBack);
	glBindTexture(GL_TEXTURE_2D, cLightingBack);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bufferWidth, bufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &cFullScene);
	glBindTexture(GL_TEXTURE_2D, cFullScene);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bufferWidth, bufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &cBackScene);
	glBindTexture(GL_TEXTURE_2D, cBackScene);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bufferWidth, bufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &cFinalScene);
	glBindTexture(GL_TEXTURE_2D, cFinalScene);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bufferWidth, bufferHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Init shader uniforms
	initializeShaders();

	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LEQUAL);
	//glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//sensor->launchUpdateThread();

	initSuccess = true;
	previousTime = glfwGetTime();
}

void Scene::update()
{
	if (!initSuccess) return;

	// Calculate per frame time interval
	currentTime = glfwGetTime();
	float frameTime = (float)(currentTime - previousTime);
	previousTime = currentTime;

	Timer::updateTimers(frameTime);

	lightingPassShader->apply();
	glUniform3fv(glGetUniformLocation(lightingPassShader->getShaderId(), "cameraPosition"), 1, value_ptr(camera->getPosition()));
	glUniform3fv(glGetUniformLocation(lightingPassShader->getShaderId(), "lightPosition"), 1, value_ptr(lightPosition));
	glUniform3fv(glGetUniformLocation(lightingPassShader->getShaderId(), "lightColor"), 1, value_ptr(vec3(lightColor.r(), lightColor.g(), lightColor.b())));
	glUniform1f(glGetUniformLocation(lightingPassShader->getShaderId(), "metallic"), metallic);
	glUniform1f(glGetUniformLocation(lightingPassShader->getShaderId(), "roughness"), roughness);
	ssr->setRoughness(roughness);

	compositeShader->apply();
	glUniform1f(glGetUniformLocation(compositeShader->getShaderId(), "exposure"), exposure);

	camera->update(frameTime);
	sensor->update();

	// Update CamMat uniform buffer
	glBindBuffer(GL_UNIFORM_BUFFER, uniform_CamMat);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), value_ptr(camera->getMatProjection()));
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4), sizeof(mat4), value_ptr(camera->getMatProjectionInverse()));
	glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(mat4), sizeof(mat4), value_ptr(camera->getMatView()));
	glBufferSubData(GL_UNIFORM_BUFFER, 3 * sizeof(mat4), sizeof(mat4), value_ptr(camera->getMatViewInverse()));
	glBufferSubData(GL_UNIFORM_BUFFER, 4 * sizeof(mat4), sizeof(mat4), value_ptr(sensor->getMatProjection()));
	glBufferSubData(GL_UNIFORM_BUFFER, 5 * sizeof(mat4), sizeof(mat4), value_ptr(sensor->getMatProjectionInverse()));

	if (timerRunOnceOnStart->ticked())
	{
		spawnDragons(1);
		pbr->recomputeEnvMaps(dsColor);
	}
}

void Scene::render(GLFWwindow* window)
{
	if (!initSuccess) return;
	if (pauseRender) return;

	// Initial GL states
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	// GBuffer pass
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glViewport(0, 0, bufferWidth, bufferHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//pointCloud->draw();
	
	gPassShader->apply();

	//plane->draw();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texWhite);
	knob->drawMeshOnly();

	// Draw dragon fest
	if (customPositions != nullptr)
	{
		for (int i = 0; i < customPositions->size(); i++)
		{
			if (customPositions->at(i) == glm::vec3(0)) continue;

			glActiveTexture(GL_TEXTURE0);
			if (i % 4 == 0) glBindTexture(GL_TEXTURE_2D, texWhite);
			else if (i % 4 == 1) glBindTexture(GL_TEXTURE_2D, texRed);
			else if (i % 4 == 2) glBindTexture(GL_TEXTURE_2D, texGreen);
			else if (i % 4 == 3) glBindTexture(GL_TEXTURE_2D, texBlue);

			dragon->setScale(vec3(0.1f) * -customPositions->at(i).z);
			float halfHeight = (dragon->getBoundingBox().Height / 2) * 0.8f;
			dragon->setPosition(customPositions->at(i) + customNormals->at(i) * halfHeight);
			//dragon->setRotationByAxisAngle(customNormals->at(i), customRandoms.at(i) * 360.0f);
			dragon->setRotationByAxisAngle(customNormals->at(i), 0.0f);
			dragon->drawMeshOnly();
		}
	}

	// Combine GBuffer and kinect buffers
	glBindFramebuffer(GL_FRAMEBUFFER, gComposeBuffer);
	glViewport(0, 0, bufferWidth, bufferHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	gComposePassShader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gColor);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, sensor->getPositionMapId());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, sensor->getNormalMapId());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, sensor->getColorMapId());

	quad->draw();

	// Compute ssao for GBuffer and kinect inputs
	ssao->drawLayer(1, gComposedPosition, gComposedNormal, gComposedColor);
	ssao->drawLayer(2, dsPosition, dsNormal, dsColor);
	ssao->drawCombined(gComposedColor);

	

	// Screen space reflection pass
	GLuint ao;
	ssr->draw(gComposedPosition, gComposedNormal, cFinalScene, pbr->getIrradianceMapId(), pbr->getPrefilterMapId(), cLightingFull, ao);
	
	// Differential rendering: Rendered (virtual) scene pass
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cFullScene, 0);

	// Lighting pass
	glViewport(0, 0, bufferWidth, bufferHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	lightingPassShader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gComposedPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gComposedNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gComposedColor);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, ssao->getTextureLayer(0));
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, pbr->getIrradianceMapId());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_CUBE_MAP, pbr->getPrefilterMapId());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, pbr->getBrdfLUTId());
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, cLightingFull);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, pbr->getReflectanceMapId());

	quad->draw();

	

	
	// Screen space reflection pass
	ssr->draw(dsPosition, dsNormal, dsColor, pbr->getIrradianceMapId(), pbr->getPrefilterMapId(), cLightingBack, ao);
	

	// Differential rendering: Background (real) scene pass
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cBackScene, 0);

	// Lighting pass
	glViewport(0, 0, bufferWidth, bufferHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	lightingPassShader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, dsPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, dsNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, dsColor);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, ssao->getTextureLayer(2));
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, pbr->getIrradianceMapId());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_CUBE_MAP, pbr->getPrefilterMapId());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, pbr->getBrdfLUTId());
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, cLightingBack);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, pbr->getReflectanceMapId());

	quad->draw();

	
	

	// Combine the differential rendering textures
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cFinalScene, 0);

	glViewport(0, 0, bufferWidth, bufferHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	compositeShader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cFullScene);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, cBackScene);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, dsColor);

	quad->draw();

	// Just output the final texture
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glViewport(0, 0, bufferWidth, bufferHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	outputShader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cFinalScene);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gComposedPosition);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gComposedNormal);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, gComposedColor);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, ssao->getTextureLayer(0));
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, dsColor);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, sensor->getDepthMapId());
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, pbr->getReflectanceMapId());

	quad->draw();


	// Draw GUI
	if (isGUIVisible)
	{
		guiScreen->drawWidgets();
		glEnable(GL_DEPTH_TEST); // Reset changed states
		glDisable(GL_BLEND);
	}

	glfwSwapBuffers(window);

	//pauseRender = true;
}

void Scene::spawnDragons(int numRadius)
{
	sensor->getCustomPixelInfo(numRadius, customPositions, customNormals);

	customRandoms.clear();
	for (int i = 0; i < customPositions->size(); i++)
	{
		customRandoms.push_back(randomFloats(generator));
	}
}



void Scene::keyCallback(int key, int action)
{
	if (nanoguiWindow != nullptr && nanoguiWindow->focused()) return;
	if (nanoguiWindow2 != nullptr && nanoguiWindow2->focused()) return;

	camera->keyCallback(key, action);

	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		recompileShaders();
	}

	// Shader modes
	if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9 && action == GLFW_PRESS)
	{
		renderMode = key - 48;
		outputShader->apply();
		glUniform1i(glGetUniformLocation(outputShader->getShaderId(), "displayMode"), renderMode);
	}

	if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{
		sensor->toggleRendering();
	}

	if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
	{
		isGUIVisible = !isGUIVisible;
	}

	if (key == GLFW_KEY_O && action == GLFW_PRESS)
	{
		pauseRender = false;
	}

	if (key == GLFW_KEY_L && action == GLFW_PRESS)
	{
		pbr->recompileShaders();
		pbr->computeReflectanceMap(gComposedNormal, cFinalScene);
	}

	if (key == GLFW_KEY_E && action == GLFW_PRESS)
	{
		pbr->recompileShaders();
		pbr->recomputeEnvMaps(dsColor);
	}

	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
	{
		spawnDragons(dragonNumRadius);
		pbr->recompileShaders();
		pbr->recomputeEnvMaps(dsColor);
	}
}

void Scene::cursorPosCallback(double x, double y)
{
	camera->cursorPosCallback(x, y);
}

void Scene::mouseCallback(int button, int action)
{
	camera->mouseCallback(button, action);
}

void Scene::windowSizeCallback(int x, int y)
{
	camera->setResolution(x, y);
}

float Scene::getRoughness() const
{
	return roughness;
}

float Scene::getMetallic() const
{
	return metallic;
}

float Scene::getExposure() const
{
	return exposure;
}

void Scene::setRoughness(float value)
{
	roughness = value;
	ssr->setRoughness(roughness);
}

void Scene::setMetallic(float value)
{
	metallic = value;
}

void Scene::setExposure(float value)
{
	exposure = value;
	lightingPassShader->apply();
	glUniform1f(glGetUniformLocation(lightingPassShader->getShaderId(), "exposure"), exposure);
}