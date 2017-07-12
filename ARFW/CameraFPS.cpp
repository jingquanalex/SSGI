#include "CameraFPS.h"

using namespace glm;
using namespace std;

extern int window_width;
extern int window_height;

CameraFPS::CameraFPS(int windowWidth, int windowHeight) : Camera(windowWidth, windowHeight)
{
	targetPoint = vec3(0);
	yaw = -90.0f;
	pitch = 0.0f;
	moveSpeed = 10.0f;
}

CameraFPS::~CameraFPS()
{

}

void CameraFPS::update(float dt)
{
	if (!isActive) return;

	Camera::update(dt);

	if (stateForward)
	{
		position += direction * moveSpeed * dt;
	}
	else if (stateBackward)
	{
		position -= direction * moveSpeed * dt;
	}

	if (stateLeft)
	{
		position -= normalize(cross(direction, up)) * moveSpeed * dt;
	}
	else if (stateRight)
	{
		position += normalize(cross(direction, up)) * moveSpeed * dt;
	}
}

void CameraFPS::keyCallback(int key, int action)
{
	if (!isActive) return;

	if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS)
	{
		moveSpeed *= 2;
	}
	else if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE)
	{
		moveSpeed = 10.0f;
	}

	switch (key)
	{
	case GLFW_KEY_W:
		stateForward = action;
		break;
	case GLFW_KEY_A:
		stateLeft = action;
		break;
	case GLFW_KEY_S:
		stateBackward = action;
		break;
	case GLFW_KEY_D:
		stateRight = action;
		break;
	}
}

// Track the delta for the mouse movements
void CameraFPS::cursorPosCallback(double x, double y)
{
	if (!isActive) return;

	Camera::cursorPosCallback(x, y);

	if (stateLookAround)
	{
		// Update yaw and pitch and limit pitch
		yaw += mouseDeltaX * mouseSensitivity;
		pitch += -mouseDeltaY * mouseSensitivity;

		//printf("%f, %f \n", yaw, pitch);

		if (pitch > 89.0f) pitch = 89.0f;
		if (pitch < -89.0f) pitch = -89.0f;

		direction.x = cos(radians(pitch)) * cos(radians(yaw));
		direction.y = sin(radians(pitch));
		direction.z = cos(radians(pitch)) * sin(radians(yaw));
		direction = normalize(direction);
	}
}

// Keys callback
void CameraFPS::mouseCallback(int button, int action)
{
	if (!isActive) return;

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		stateLookAround = true;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
	{
		stateLookAround = false;
	}
}

void CameraFPS::setTargetPoint(vec3 position)
{
	this->targetPoint = position;
	direction = normalize(this->targetPoint - this->position);
	// TODO: wrong angles
	pitch = degrees(atan2(direction.x, direction.y));
	yaw = degrees(asin(direction.z));
	//cout << "yaw: " << yaw << " pitch: " << pitch << endl;
	updateViewMatrix();
}

void CameraFPS::setMoveSpeed(float speed)
{
	moveSpeed = speed;
}