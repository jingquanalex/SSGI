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

void CameraFPS::mouse(int button, int state)
{
	if (!isActive) return;

	/*if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
	{
		stateLookAround = true;
	}
	else if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP)
	{
		stateLookAround = false;
	}*/
}

// Track the delta for the mouse movements
void CameraFPS::mouseMotion(int x, int y)
{
	if (!isActive) return;

	if (stateLookAround)
	{
		Camera::mouseMotion(x, y);

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
void CameraFPS::keyboard(int key)
{
	if (!isActive) return;

	/*if (glutGetModifiers() & GLUT_ACTIVE_SHIFT)
	{
		moveSpeed *= 2;
	}
	else
	{
		moveSpeed = 10.0f;
	}

	switch (key)
	{
	case 'w':
		stateForward = true;
		break;
	case 'a':
		stateLeft = true;
		break;
	case 's':
		stateBackward = true;
		break;
	case 'd':
		stateRight = true;
		break;
	}*/
}

// Keys up callback
void CameraFPS::keyboardUp(int key)
{
	if (!isActive) return;

	switch (key)
	{
	case 'w':
		stateForward = false;
		break;
	case 'a':
		stateLeft = false;
		break;
	case 's':
		stateBackward = false;
		break;
	case 'd':
		stateRight = false;
		break;
	}
}

void CameraFPS::setTargetPoint(vec3 position)
{
	this->targetPoint = position;
	direction = normalize(this->targetPoint - this->position);
	// TODO: wrong angles
	yaw = degrees(atan2(direction.x, direction.y));
	pitch = degrees(asin(direction.z));
	updateViewMatrix();
}