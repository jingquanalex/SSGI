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
	delete quadShader;
}

void Scene::initialize()
{
	camera = new CameraFPS(g_windowWidth, g_windowHeight);
	camera->setActive(true);
	quadShader = new Shader("quad");
	quad = new Quad();
	sponza = new Object();
	sponza->setGPassShaderId(quadShader->getShaderId());
	//sponza->setScale(vec3(0.1f));
	//sponza->load("sponza/sponza.obj");
	sponza->load("sibenik/sibenik.obj");
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

	glEnable(GL_DEPTH_TEST);
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
	glViewport(0, 0, g_windowWidth, g_windowHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	quadShader->apply();
	glUniform1i(glGetUniformLocation(quadShader->getShaderId(), "diffuse1"), 0);
	glBindTexture(GL_TEXTURE_2D, tex);
	quad->draw();
	sponza->draw();
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