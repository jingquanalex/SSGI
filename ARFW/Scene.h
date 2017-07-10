#pragma once

#include "global.h"
#include <nanogui\nanogui.h>
#include "Timer.h"
#include "Shader.h"
#include "CameraFPS.h"
#include "Quad.h"
#include "SSAO.h"
#include "DSensor.h"
#include "PBR.h"
#include "PointCloud.h"

#include <chrono>
#include <thread>

class Scene
{
	
private:

	bool initSuccess = false;
	double currentTime, previousTime = 0.0;

	GLuint gBuffer, gBlendBuffer;
	GLuint gPosition, gNormal, gColor, gDepth;
	GLuint gBlendPosition, gBlendNormal, gBlendColor, gBlendDepth;
	Shader* gPassShader = nullptr;
	Shader* lightingPassShader = nullptr;
	Shader* gBlendPassShader = nullptr;
	Shader* gMedianPassShader = nullptr;
	SSAO* ssao = nullptr;
	float ssaoKernelRadius = 1.35f;
	float ssaoSampleBias = 0.0f;
	int bufferWidth, bufferHeight;

	CameraFPS* camera = nullptr;
	Quad* quad = nullptr;
	Object* sponza = nullptr;
	Object* plane;
	GLuint texMask;

	GLuint uniform_CamMat;
	int renderMode = 1;

	nanogui::Screen* guiScreen = nullptr;
	nanogui::FormHelper *gui = nullptr;
	nanogui::ref<nanogui::Window> nanoguiWindow = nullptr;

	DSensor* sensor = nullptr;

	glm::vec3 lightPosition = glm::vec3(0.0f, 1.5f, 0.0f);
	nanogui::Color lightColor = nanogui::Color(1.00f, 0.48f, 0.49f, 1.0f);
	float roughness = 0.24f;
	float metallic = 0.1f;

	PBR* pbr;
	GLuint environmentMap;
	GLuint irradianceMap;
	GLuint prefilterMap;
	GLuint brdfLUT;

	Shader* compositeShader;
	GLuint captureFBO;
	GLuint captureRBO;
	GLuint cFullScene;
	GLuint cPlaneScene;

	PointCloud* pointCloud;

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