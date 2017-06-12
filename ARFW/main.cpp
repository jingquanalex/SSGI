#pragma once
#include "Scene.h"

std::string g_ExePath;
int g_windowWidth = 1920;
int g_windowHeight = 1080;
Scene scene;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void cursorPosCallback(GLFWwindow* window, double x, double y);
void mouseCallback(GLFWwindow* window, int button, int action, int mods);
void windowSizeCallback(GLFWwindow* window, int x, int y);

int main(int argc, char *argv[])
{
	std::string exePath(argv[0]);
	g_ExePath = exePath.substr(0, exePath.find_last_of("\\/")) + "\\";

	glfwInit();
	GLFWwindow* window = glfwCreateWindow(g_windowWidth, g_windowHeight, "ARFW", NULL, NULL);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	glfwSetKeyCallback(window, keyCallback);
	glfwSetCursorPosCallback(window, cursorPosCallback);
	glfwSetMouseButtonCallback(window, mouseCallback);
	glfwSetWindowSizeCallback(window, windowSizeCallback);
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glewInit();
	scene.initialize();

	while (!glfwWindowShouldClose(window))
	{
		scene.update();
		scene.render();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

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

	scene.keyCallback(key, action);
}

static void cursorPosCallback(GLFWwindow* window, double x, double y)
{
	scene.cursorPosCallback(x, y);
}

static void mouseCallback(GLFWwindow* window, int button, int action, int mods)
{
	scene.mouseCallback(button, action);
}

static void windowSizeCallback(GLFWwindow* window, int x, int y)
{
	g_windowWidth = x;
	g_windowHeight = y;
	scene.windowSizeCallback(x, y);
}