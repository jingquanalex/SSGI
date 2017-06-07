#include "Scene.h"

Scene::Scene()
{
}

Scene::~Scene()
{
	delete testShader;
}

void Scene::initialize()
{
	testShader = new Shader("test");
}

void Scene::update()
{
}

void Scene::render()
{
}

void Scene::keyCallback(int key, int action)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		
	}
}
