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
	if (sponza != nullptr) delete sponza;
	if (ssao != nullptr) delete ssao;
}

void Scene::recompileShaders()
{
	gPassShader->recompile();
	gBlendPassShader->recompile();
	gMedianPassShader->recompile();
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

	gBlendPassShader->apply();
	glUniform1i(glGetUniformLocation(gBlendPassShader->getShaderId(), "gInPosition"), 0);
	glUniform1i(glGetUniformLocation(gBlendPassShader->getShaderId(), "gInNormal"), 1);
	glUniform1i(glGetUniformLocation(gBlendPassShader->getShaderId(), "gInColor"), 2);
	glUniform1i(glGetUniformLocation(gBlendPassShader->getShaderId(), "dsColor"), 3);
	glUniform1i(glGetUniformLocation(gBlendPassShader->getShaderId(), "dsDepth"), 4);

	gMedianPassShader->apply();
	glUniform1i(glGetUniformLocation(gMedianPassShader->getShaderId(), "gInPosition"), 0);
	glUniform1i(glGetUniformLocation(gMedianPassShader->getShaderId(), "gInNormal"), 1);
	glUniform1i(glGetUniformLocation(gMedianPassShader->getShaderId(), "gInColor"), 2);

	lightingPassShader->apply();
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "displayMode"), renderMode);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "gPosition"), 0);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "gNormal"), 1);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "gColor"), 2);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "gDepth"), 3);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "texNoise"), 4);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "dsColor"), 5);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "dsDepth"), 6);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "irradianceMap"), 7);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "prefilterMap"), 8);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "brdfLUT"), 9);
	glUniform3fv(glGetUniformLocation(lightingPassShader->getShaderId(), "samples"), 64, value_ptr(ssao->getKernel()[0]));

	compositeShader->apply();
	glUniform1i(glGetUniformLocation(compositeShader->getShaderId(), "fullScene"), 0);
	glUniform1i(glGetUniformLocation(compositeShader->getShaderId(), "planeScene"), 1);
	glUniform1i(glGetUniformLocation(compositeShader->getShaderId(), "dsColor"), 2);
}

void Scene::initialize(nanogui::Screen* guiScreen)
{
	// Load framework objects
	bufferWidth = g_windowWidth;
	bufferHeight = g_windowHeight;
	camera = new CameraFPS(g_windowWidth, g_windowHeight);
	camera->setActive(true);
	camera->setPosition(vec3(0, 0, 0));
	camera->setDirection(vec3(0, 0, -1));
	gPassShader = new Shader("gPass");
	if (gPassShader->hasError()) return;
	lightingPassShader = new Shader("lightingPass");
	if (lightingPassShader->hasError()) return;
	gBlendPassShader = new Shader("gBlendPass");
	gMedianPassShader = new Shader("gMedianPass");

	quad = new Quad();
	plane = new Object();
	plane->setGPassShaderId(gPassShader->getShaderId());
	plane->setPosition(vec3(0.0f, -0.75f, -4.0f));
	plane->setScale(vec3(1.0f, 0.1f, 1.0f));
	plane->load("cube/cube.obj");
	sponza = new Object();
	sponza->setGPassShaderId(gPassShader->getShaderId());
	//sponza->setScale(vec3(0.05f));
	sponza->setScale(vec3(2.5f));
	sponza->setPosition(vec3(0, 0, -4));
	sponza->setRotation(vec3(0, 90, 0));
	sponza->load("dragon.obj");
	//sponza->load("sibenik/sibenik.obj");
	//sponza->load("sponza/sponza.obj");
	texMask = Image::loadTexture(g_ExePath + "../../media/diff.png");

	// Initialize depth sensor
	sensor = new DSensor();
	sensor->initialize(g_windowWidth, g_windowHeight);

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

	gui->addGroup("Light");
	gui->addVariable("Position X", lightPosition.x);
	gui->addVariable("Position Y", lightPosition.y);
	gui->addVariable("Position Z", lightPosition.z);
	gui->addVariable("Color", lightColor);
	gui->addVariable("Roughness", roughness);
	gui->addVariable("Metallic", metallic);

	gui->addGroup("SSAO");
	gui->addVariable("ssaoKernelRadius", ssaoKernelRadius);
	gui->addVariable("ssaoSampleBias", ssaoSampleBias);

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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, bufferWidth, bufferHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gDepth, 0);

	const GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	// Create buffers for gBlend pass TODO: optimize texture array and glTextureBarrier
	glGenFramebuffers(1, &gBlendBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBlendBuffer);

	glGenTextures(1, &gBlendPosition);
	glBindTexture(GL_TEXTURE_2D, gBlendPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gBlendPosition, 0);

	glGenTextures(1, &gBlendNormal);
	glBindTexture(GL_TEXTURE_2D, gBlendNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, bufferWidth, bufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gBlendNormal, 0);

	glGenTextures(1, &gBlendColor);
	glBindTexture(GL_TEXTURE_2D, gBlendColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufferWidth, bufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gBlendColor, 0);

	glDrawBuffers(3, attachments);

	// Capture final outputs for differential rendering
	compositeShader = new Shader("composite");

	glGenFramebuffers(1, &captureFBO);
	glGenRenderbuffers(1, &captureRBO);
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, bufferWidth, bufferHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

	glGenTextures(1, &cFullScene);
	glBindTexture(GL_TEXTURE_2D, cFullScene);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufferWidth, bufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &cPlaneScene);
	glBindTexture(GL_TEXTURE_2D, cPlaneScene);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufferWidth, bufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Init SSAO shader data
	ssao = new SSAO();

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
}

void Scene::update()
{
	if (!initSuccess) return;

	// Calculate per frame time interval
	currentTime = glfwGetTime();
	float frameTime = (float)(currentTime - previousTime);
	previousTime = currentTime;


	lightingPassShader->apply();
	glUniform1f(glGetUniformLocation(lightingPassShader->getShaderId(), "screenWidth"), (float)g_windowWidth);
	glUniform1f(glGetUniformLocation(lightingPassShader->getShaderId(), "screenHeight"), (float)g_windowHeight);
	glUniform1f(glGetUniformLocation(lightingPassShader->getShaderId(), "kernelRadius"), ssaoKernelRadius);
	glUniform1f(glGetUniformLocation(lightingPassShader->getShaderId(), "sampleBias"), ssaoSampleBias);

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

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texMask);

	sponza->draw();
	plane->drawMeshOnly();

	// Blend G-Buffer and kinect buffers
	glBindFramebuffer(GL_FRAMEBUFFER, gBlendBuffer);
	glViewport(0, 0, bufferWidth, bufferHeight);
	glClear(GL_COLOR_BUFFER_BIT);

	gBlendPassShader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gColor);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, sensor->getColorMapId());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, sensor->getDepthMapId());

	quad->draw();

	// Median filter pass
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glViewport(0, 0, bufferWidth, bufferHeight);
	glClear(GL_COLOR_BUFFER_BIT);

	gMedianPassShader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBlendPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gBlendNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gBlendColor);

	quad->draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cFullScene, 0);

	// Lighting pass
	glViewport(0, 0, g_windowWidth, g_windowHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	lightingPassShader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gColor);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, gDepth);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, ssao->getNoiseTexId());

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, sensor->getColorMapId());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, sensor->getDepthMapId());

	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_2D, brdfLUT);

	quad->draw();

	// 2nd render planescene
	// G-Buffer pass
	/*glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glViewport(0, 0, bufferWidth, bufferHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	gPassShader->apply();
	//sponza->draw();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texMask);
	plane->drawMeshOnly();

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cPlaneScene, 0);

	// Lighting pass
	glViewport(0, 0, g_windowWidth, g_windowHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	lightingPassShader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gColor);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, gDepth);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, ssao->getNoiseTexId());

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, sensor->getColorMapId());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, sensor->getDepthMapId());

	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_2D, brdfLUT);

	quad->draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glViewport(0, 0, g_windowWidth, g_windowHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	compositeShader->apply();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cFullScene);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, cPlaneScene);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, sensor->getColorMapId());

	quad->draw();*/

	// Draw GUI
	guiScreen->drawWidgets();
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND); //fucks up pos and norm gtex
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