#include "Scene.h"

std::string g_ExePath;
Scene scene;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

int main(int argc, char *argv[])
{
	std::string exePath(argv[0]);
	g_ExePath = exePath.substr(0, exePath.find_last_of("\\/")) + "\\";

	glfwInit();
	GLFWwindow* window = glfwCreateWindow(1920, 1080, "ARFW", NULL, NULL);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	glfwSetKeyCallback(window, keyCallback);

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