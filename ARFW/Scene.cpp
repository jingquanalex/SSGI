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
	sensor->recompileShaders();
	gPassShader->recompile();
	gComposePassShader->recompile();
	ssao->recompileShaders();
	lightingPassShader->recompile();
	compositeShader->recompile();
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
	glUniform1i(glGetUniformLocation(gComposePassShader->getShaderId(), "dsColor"), 3);
	glUniform1i(glGetUniformLocation(gComposePassShader->getShaderId(), "dsPosition"), 4);
	glUniform1i(glGetUniformLocation(gComposePassShader->getShaderId(), "dsNormal"), 5);

	lightingPassShader->apply();
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "displayMode"), renderMode);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "gPosition"), 0);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "gNormal"), 1);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "gColor"), 2);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "aoMap"), 3);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "dsColor"), 4);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "dsDepth"), 5);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "irradianceMap"), 6);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "prefilterMap"), 7);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "brdfLUT"), 8);

	compositeShader->apply();
	glUniform1i(glGetUniformLocation(compositeShader->getShaderId(), "fullScene"), 0);
	glUniform1i(glGetUniformLocation(compositeShader->getShaderId(), "backScene"), 1);
	glUniform1i(glGetUniformLocation(compositeShader->getShaderId(), "dsColor"), 2);
}

void Scene::initialize(nanogui::Screen* guiScreen)
{
	// Load framework objects
	bufferWidth = g_windowWidth;
	bufferHeight = g_windowHeight;
	camera = new CameraFPS(g_windowWidth, g_windowHeight);
	camera->setActive(true);
	camera->setMoveSpeed(1);
	camera->setPosition(vec3(0, 0, 0));
	camera->setDirection(vec3(0, 0, -1));
	gPassShader = new Shader("gPass");
	lightingPassShader = new Shader("lightingPass");
	gComposePassShader = new Shader("gComposePass");

	ssao = new SSAO(g_windowWidth, g_windowHeight);
	quad = new Quad();
	plane = new Object();
	plane->setGPassShaderId(gPassShader->getShaderId());
	plane->setPosition(vec3(0.0f, -0.75f, -4.0f));
	plane->setScale(vec3(1.0f, 0.1f, 1.0f));
	plane->load("cube/cube.obj");
	dragon = new Object();
	dragon->setGPassShaderId(gPassShader->getShaderId());
	//dragon->setScale(vec3(0.05f));
	dragon->setScale(vec3(0.25f));
	dragon->setPosition(vec3(0, 0, -0.5));
	dragon->setRotation(vec3(0, 90, 0));
	dragon->load("dragon.obj");
	//dragon->load("sibenik/sibenik.obj");
	//dragon->load("dragon/dragon.obj");
	texWhite = Image::loadTexture(g_ExePath + "../../media/white.png");
	texRed = Image::loadTexture(g_ExePath + "../../media/red.png");
	texGreen = Image::loadTexture(g_ExePath + "../../media/green.png");
	texBlue = Image::loadTexture(g_ExePath + "../../media/blue.png");

	// Initialize depth sensor
	sensor = new DSensor();
	sensor->initialize(g_windowWidth, g_windowHeight);

	randomFloats = uniform_real_distribution<float>(0.0f, 1.0f);

	pointCloud = new PointCloud(sensor->getColorMapId(), sensor->getDepthMapId());

	// Initialize GUI
	this->guiScreen = guiScreen;
	gui = new nanogui::FormHelper(guiScreen);
	nanoguiWindow = gui->addWindow(Eigen::Vector2i(10, 10), "ARFW");

	gui->addGroup("Shader");
	gui->addButton("Recompile", [&]()
	{
		recompileShaders();
	});

	gui->addGroup("Camera");
	gui->addButton("Reset Orientation", [&]()
	{
		camera->setPosition(vec3(0, 0, 0));
		//camera->setDirection(vec3(0, 0, -1));
		camera->setTargetPoint(camera->getPosition() + vec3(0, 0, -1));
	});

	gui->addGroup("Misc");
	gui->addVariable("dragonNumRadius", dragonNumRadius);
	gui->addButton("Spawn Dragons", [&]()
	{
		spawnDragons(dragonNumRadius);
	});

	gui->addGroup("Light");
	gui->addVariable("Position X", lightPosition.x);
	gui->addVariable("Position Y", lightPosition.y);
	gui->addVariable("Position Z", lightPosition.z);
	gui->addVariable("Color", lightColor);
	gui->addVariable("Roughness", roughness);
	gui->addVariable("Metallic", metallic);

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
	gui->addVariable<float>("bsigma",
		[&](const float &value) { sensor->setBlurBSigma(value); },
		[&]() { return sensor->getBlurBSigma(); });

	gui->addGroup("SSAO");
	gui->addVariable<float>("kernelRadius",
		[&](const float &value) { ssao->setKernelRadius(value); },
		[&]() { return ssao->getKernelRadius(); });
	gui->addVariable<float>("sampleBias",
		[&](const float &value) { ssao->setSampleBias(value); },
		[&]() { return ssao->getSampleBias(); });
	gui->addVariable<float>("intensity",
		[&](const float &value) { ssao->setIntensity(value); },
		[&]() { return ssao->getIntensity(); });
	gui->addVariable<float>("power",
		[&](const float &value) { ssao->setPower(value); },
		[&]() { return ssao->getPower(); });

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
	pbr = new PBR();
	pbr->precomputeEnvMaps(environmentMap, irradianceMap, prefilterMap, brdfLUT);

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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufferWidth, bufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
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

	const GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	// Create buffers for gBlend pass
	glGenFramebuffers(1, &gBlendBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBlendBuffer);

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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufferWidth, bufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gComposedColor, 0);

	glDrawBuffers(3, attachments);

	// Capture final outputs for differential rendering
	compositeShader = new Shader("composite");

	glGenFramebuffers(1, &captureFBO);

	glGenTextures(1, &cFullScene);
	glBindTexture(GL_TEXTURE_2D, cFullScene);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufferWidth, bufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &cBackScene);
	glBindTexture(GL_TEXTURE_2D, cBackScene);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufferWidth, bufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	

	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// Init shader uniforms
	initializeShaders();

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LEQUAL);
	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	initSuccess = true;

	//sensor->launchUpdateThread();
}

void Scene::update()
{
	if (!initSuccess) return;

	// Calculate per frame time interval
	currentTime = glfwGetTime();
	float frameTime = (float)(currentTime - previousTime);
	previousTime = currentTime;

	lightingPassShader->apply();
	glUniform3fv(glGetUniformLocation(lightingPassShader->getShaderId(), "cameraPosition"), 1, value_ptr(camera->getPosition()));
	glUniform3fv(glGetUniformLocation(lightingPassShader->getShaderId(), "lightPosition"), 1, value_ptr(lightPosition));
	glUniform3fv(glGetUniformLocation(lightingPassShader->getShaderId(), "lightColor"), 1, value_ptr(vec3(lightColor.r(), lightColor.g(), lightColor.b())));
	glUniform1f(glGetUniformLocation(lightingPassShader->getShaderId(), "metallic"), metallic);
	glUniform1f(glGetUniformLocation(lightingPassShader->getShaderId(), "roughness"), roughness);

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

	if (runAfterFramesCounter < runAfterFrames)
	{
		runAfterFramesCounter++;
		if (runAfterFramesCounter == runAfterFrames)
		{
			spawnDragons(1);
		}
	}
}

void Scene::render()
{
	if (!initSuccess) return;

	// G-Buffer pass
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glViewport(0, 0, bufferWidth, bufferHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//pointCloud->draw();
	
	gPassShader->apply();

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

			dragon->setScale(vec3(0.1f));
			dragon->setPosition(customPositions->at(i));
			dragon->setRotation(customRotations.at(i));
			dragon->drawMeshOnly();
		}
	}

	// Combine G-Buffer and kinect buffers
	glBindFramebuffer(GL_FRAMEBUFFER, gBlendBuffer);
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
	glBindTexture(GL_TEXTURE_2D, sensor->getColorMapId());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, sensor->getPositionMapId());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, sensor->getNormalMapId());

	quad->draw();

	ssao->drawLayer(1, gComposedPosition, gComposedNormal, gComposedColor);
	ssao->drawLayer(2, sensor->getPositionMapId(), sensor->getNormalMapId(), sensor->getColorMapId());
	ssao->drawCombined(gComposedColor);
	
	// draw to buffer for differential rendering
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cFullScene, 0);

	// Lighting pass
	glViewport(0, 0, g_windowWidth, g_windowHeight);
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
	glBindTexture(GL_TEXTURE_2D, sensor->getColorMapId());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, sensor->getDepthMapId());

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, brdfLUT);

	quad->draw();

	

	// Differential rendering, masked object scene
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cBackScene, 0);

	// Lighting pass
	glViewport(0, 0, g_windowWidth, g_windowHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	lightingPassShader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sensor->getPositionMapId());
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, sensor->getNormalMapId());
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, sensor->getColorMapId());
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, ssao->getTextureLayer(2));

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, sensor->getColorMapId());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, sensor->getDepthMapId());

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, brdfLUT);

	quad->draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glViewport(0, 0, g_windowWidth, g_windowHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	compositeShader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cFullScene);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, cBackScene);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, sensor->getColorMapId());

	quad->draw();

	// Draw GUI
	guiScreen->drawWidgets();
	glEnable(GL_DEPTH_TEST); // Reset changed states
	glDisable(GL_BLEND);
}

void Scene::spawnDragons(int numRadius)
{
	customPositions = sensor->getCustomPositions(numRadius);
	customRotations.clear();
	for (int i = 0; i < customPositions->size(); i++)
	{
		customRotations.push_back(vec3(randomFloats(generator) * 360, randomFloats(generator) * 360, randomFloats(generator) * 360));
	}
}

void Scene::keyCallback(int key, int action)
{
	if (nanoguiWindow != nullptr && nanoguiWindow->focused()) return;

	camera->keyCallback(key, action);

	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		recompileShaders();
	}

	// Shader modes
	if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9 && action == GLFW_PRESS)
	{
		renderMode = key - 48;
		lightingPassShader->apply();
		glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "displayMode"), renderMode);
	}

	if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{
		sensor->toggleRendering();
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