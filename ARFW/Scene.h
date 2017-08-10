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
#include "SSReflection.h"

#include <random>

class Scene
{
	
private:

	bool initSuccess = false;
	double currentTime, previousTime = 0.0;
	bool pauseRender = false;

	GLuint gBuffer, gComposeBuffer;
	GLuint gPosition, gNormal, gColor, gDepth;
	GLuint gComposedPosition, gComposedNormal, gComposedColor;
	GLuint dsPosition, dsNormal, dsColor;
	Shader* gPassShader = nullptr;
	Shader* lightingPassShader = nullptr;
	Shader* gComposePassShader = nullptr;
	SSAO* ssao = nullptr;
	int bufferWidth, bufferHeight;

	CameraFPS* camera = nullptr;
	Quad* quad = nullptr;
	Object* dragon = nullptr;
	Object* plane;
	GLuint texWhite, texRed, texGreen, texBlue;

	std::uniform_real_distribution<float> randomFloats;
	std::default_random_engine generator;
	std::vector<glm::vec3>* customPositions = nullptr;
	std::vector<glm::vec3>* customNormals = nullptr;
	std::vector<float> customRandoms;
	Timer* timerRunOnceOnStart;

	GLuint uniform_CamMat;
	int renderMode = 1;

	nanogui::Screen* guiScreen = nullptr;
	nanogui::FormHelper *gui = nullptr;
	nanogui::ref<nanogui::Window> nanoguiWindow = nullptr;
	nanogui::ref<nanogui::Window> nanoguiWindow2 = nullptr;
	bool isGUIVisible = true;

	DSensor* sensor = nullptr;

	glm::vec3 lightPosition = glm::vec3(0.0f, 1111.5f, 0.0f);
	nanogui::Color lightColor = nanogui::Color(1.00f, 0.48f, 0.49f, 1.0f);
	float roughness = 0.24f;
	float metallic = 0.1f;
	float exposure = 2.0;

	PBR* pbr;

	Shader* compositeShader;
	Shader* outputShader;
	GLuint captureFBO;
	GLuint cLightingFull, cLightingBack;
	GLuint cFullScene, cBackScene;
	GLuint cFinalScene;
	int currFrame = 0;

	SSReflection* ssr;
	

	PointCloud* pointCloud;

	void recompileShaders();
	void initializeShaders();

	void spawnDragons(int numRadius);
	int dragonNumRadius = 1;

public:

	Scene();
	~Scene();

	void initialize(nanogui::Screen* guiScreen);
	void update();
	void render(GLFWwindow* window);

	void keyCallback(int key, int action);
	void cursorPosCallback(double x, double y);
	void mouseCallback(int button, int action);
	void windowSizeCallback(int x, int y);

	float getRoughness() const;
	float getMetallic() const;
	float getExposure() const;

	void setRoughness(float value);
	void setMetallic(float value);
	void setExposure(float value);

};