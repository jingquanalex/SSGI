#pragma once

#include "global.h"
#include "Timer.h"
#include "Shader.h"

class Scene
{
	
private:

	Shader* testShader;

public:

	Scene();
	~Scene();

	void initialize();
	void update();
	void render();

	void keyCallback(int key, int action);

};