#include "Scene.h"

std::string g_ExePath;
int g_windowWidth = 1920;
int g_windowHeight = 1080;

nanogui::Screen* guiScreen;
Scene* scene;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void cursorPosCallback(GLFWwindow* window, double x, double y);
void mouseCallback(GLFWwindow* window, int button, int action, int mods);
void windowSizeCallback(GLFWwindow* window, int w, int h);
void charCallback(GLFWwindow* window, unsigned int codepoint);
void dropCallback(GLFWwindow* window, int count, const char** paths);
void scrollCallback(GLFWwindow* window, double x, double y);

int main(int argc, char *argv[])
{
	std::string exePath(argv[0]);
	g_ExePath = exePath.substr(0, exePath.find_last_of("\\/")) + "\\";

	glfwInit();
	GLFWwindow* window = glfwCreateWindow(g_windowWidth, g_windowHeight, "ARFW", NULL, NULL);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	glfwWindowHint(GLFW_DEPTH_BITS, 24);

	glewInit();

	guiScreen = new nanogui::Screen();
	guiScreen->initialize(window, false);
	scene = new Scene();
	scene->initialize(guiScreen);

	glfwSetKeyCallback(window, keyCallback);
	glfwSetCursorPosCallback(window, cursorPosCallback);
	glfwSetMouseButtonCallback(window, mouseCallback);
	glfwSetWindowSizeCallback(window, windowSizeCallback);
	glfwSetCharCallback(window, charCallback);
	glfwSetDropCallback(window, dropCallback);
	glfwSetScrollCallback(window, scrollCallback);
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		scene->update();
		scene->render(window);
	}

	delete scene;
	delete guiScreen;
	glfwDestroyWindow(window);
	glfwTerminate();
	
	return 0;
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}

	scene->keyCallback(key, action);
	guiScreen->keyCallbackEvent(key, scancode, action, mods);
}

static void cursorPosCallback(GLFWwindow* window, double x, double y)
{
	scene->cursorPosCallback(x, y);
	guiScreen->cursorPosCallbackEvent(x, y);
}

static void mouseCallback(GLFWwindow* window, int button, int action, int mods)
{
	scene->mouseCallback(button, action);
	guiScreen->mouseButtonCallbackEvent(button, action, mods);
}

static void windowSizeCallback(GLFWwindow* window, int w, int h)
{
	g_windowWidth = w;
	g_windowHeight = h;
	scene->windowSizeCallback(w, h);
	guiScreen->resizeCallbackEvent(w, h);
}

static void charCallback(GLFWwindow* window, unsigned int codepoint)
{
	guiScreen->charCallbackEvent(codepoint);
}

static void dropCallback(GLFWwindow* window, int count, const char** paths)
{
	guiScreen->dropCallbackEvent(count, paths);
}

static void scrollCallback(GLFWwindow* window, double x, double y)
{
	guiScreen->scrollCallbackEvent(x, y);
}