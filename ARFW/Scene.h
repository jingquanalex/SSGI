#pragma once

#include "global.h"
#include "Timer.h"
#include "Shader.h"
#include "CameraFPS.h"
#include "Quad.h"

class Scene
{
	
private:

	double currentTime, previousTime = 0.0;

	Shader* quadShader;
	Camera* camera;
	Quad* quad;

	GLuint uniform_CamMat;

public:

	Scene();
	~Scene();

	void initialize();
	void update();
	void render();

	void keyCallback(int key, int action);
	void cursorPosCallback(double x, double y);
	void mouseCallback(int button, int action);
	void windowSizeCallback(int x, int y);

};