#pragma once

#include "global.h"
#include <nanogui\nanogui.h>
#include "Timer.h"
#include "Shader.h"
#include "CameraFPS.h"
#include "Quad.h"
#include "SSAO.h"
#include "DSensor.h"

#include <chrono>
#include <thread>

class Scene
{
	
private:

	bool initSuccess = false;
	double currentTime, previousTime = 0.0;

	GLuint gBuffer, gPosition, gNormal, gColor, gDepth;
	Shader* gPassShader = nullptr;
	Shader* lightingPassShader = nullptr;
	SSAO* ssao = nullptr;
	float ssaoKernelRadius = 10.35f;
	float ssaoSampleBias = 0.1f;
	int bufferWidth, bufferHeight;

	Camera* camera = nullptr;
	Quad* quad = nullptr;
	Object* sponza = nullptr;
	GLuint tex;

	GLuint uniform_CamMat;
	int renderMode = 1;

	nanogui::Screen* guiScreen = nullptr;
	nanogui::FormHelper *gui = nullptr;
	nanogui::ref<nanogui::Window> nanoguiWindow = nullptr;

	DSensor* sensor = nullptr;

	void recompileShaders();
	void initializeShaders();

public:

	Scene();
	~Scene();

	void initialize(nanogui::Screen* guiScreen);
	void update();
	void render();

	void keyCallback(int key, int action);
	void cursorPosCallback(double x, double y);
	void mouseCallback(int button, int action);
	void windowSizeCallback(int x, int y);

};