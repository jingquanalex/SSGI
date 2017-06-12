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
	delete camera;
	delete gPassShader;
	delete lightingPassShader;
	delete quad;
	delete sponza;
}

void Scene::initialize()
{
	camera = new CameraFPS(g_windowWidth, g_windowHeight);
	camera->setActive(true);
	gPassShader = new Shader("gPass");
	lightingPassShader = new Shader("lightingPass");
	quad = new Quad();
	sponza = new Object();
	sponza->setGPassShaderId(gPassShader->getShaderId());
	sponza->setScale(vec3(0.05f));
	sponza->load("sponza/sponza.obj");
	//sponza->load("sibenik/sibenik.obj");
	tex = loadTexture(g_ExePath + "../../media/sibenik/kamen.png");

	// Initialize CamMat uniform buffer
	glGenBuffers(1, &uniform_CamMat);
	glBindBuffer(GL_UNIFORM_BUFFER, uniform_CamMat);
	glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(mat4), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniform_CamMat);

	// Set matrices to identity
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), value_ptr(mat4(1)));
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4), sizeof(mat4), value_ptr(mat4(1)));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// Deferred rendering buffer
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, g_windowWidth, g_windowHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, g_windowWidth, g_windowHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

	glGenTextures(1, &gColor);
	glBindTexture(GL_TEXTURE_2D, gColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_windowWidth, g_windowHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gColor, 0);

	const GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, g_windowWidth, g_windowHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Set lightingPass texture binding ids
	lightingPassShader->apply();
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "gPosition"), 0);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "gNormal"), 1);
	glUniform1i(glGetUniformLocation(lightingPassShader->getShaderId(), "gColor"), 2);

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
}

void Scene::update()
{
	// Calculate per frame time interval
	currentTime = glfwGetTime();
	float frameTime = (float)(currentTime - previousTime);
	previousTime = currentTime;

	camera->update(frameTime);

	// Update uniform shader variables for CamMat
	glBindBuffer(GL_UNIFORM_BUFFER, uniform_CamMat);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), value_ptr(camera->getMatProjection()));
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4), sizeof(mat4), value_ptr(camera->getMatView()));
}

void Scene::render()
{
	// Draw to gBuffer
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glViewport(0, 0, g_windowWidth, g_windowHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	gPassShader->apply();
	sponza->draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Draw to quad
	glViewport(0, 0, g_windowWidth, g_windowHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	lightingPassShader->apply();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gColor);
	quad->draw();

	// Copy gBuffer's depth to framebuffer's
	/*glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, g_windowWidth, g_windowHeight, 0, 0, g_windowWidth, g_windowHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);*/
}

void Scene::keyCallback(int key, int action)
{
	camera->keyCallback(key, action);
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