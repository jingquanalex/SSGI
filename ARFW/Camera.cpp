#include "Camera.h"

using namespace glm;
using namespace std;

// Input: width and height of the window as int.
Camera::Camera(int windowWidth, int windowHeight)
{
	isWrappingPointer = false;
	mouseTriggered = false;
	isActive = false;
	isOrtho = false;
	mouseLastX = 0;
	mouseLastY = 0;
	position = vec3(0.0f, 3.0f, 5.0f);
	direction = vec3(0.0f, 0.0f, -1.0f);
	up = vec3(0.0f, 1.0f, 0.0f);
	fov = 75.0f;
	zNear = 1.0f;
	zFar = 50.0f;

	mouseSensitivity = 0.25f;
	smoothness = 50.0f;

	setResolution(windowWidth, windowHeight);
	updateViewMatrix();
}

Camera::~Camera()
{

}

// Input: The frame delta time.
// Call in main update loop.
void Camera::update(float dt)
{
	if (!isActive) return;

	updateViewMatrix();
}

// Create projection matrix, maintain viewport aspect ratio.
void Camera::updateProjectionMatrix()
{
	if (isOrtho)
	{
		float width = 90.0f;
		float height = 80.0f;
		matProjection = ortho(-width, width, -height, height, zNear, zFar);
	}
	else
	{
		matProjection = perspective(radians(fov), aspectRatio, zNear, zFar);
	}

	matInvProjection = inverse(matProjection);
}

// Create view matrix based on position.
void Camera::updateViewMatrix()
{
	matView = lookAt(position, position + direction, up);
}

void Camera::mouse(int button, int state)
{

}

// Track the delta for the mouse movements
void Camera::mouseMotion(int x, int y)
{
	/*if (!mouseTriggered)
	{
		mouseX = x;
		mouseY = y;
	}
	else
	{
		if (!isWrappingPointer)
		{
			mouseX = x;
			mouseY = y;

			if (mouseX != glutGet(GLUT_WINDOW_WIDTH) / 2 || mouseY != glutGet(GLUT_WINDOW_HEIGHT) / 2)
			{
				isWrappingPointer = true;
				glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);
			}
		}
		else
		{
			isWrappingPointer = false;
			mouseLastX = glutGet(GLUT_WINDOW_WIDTH) / 2;
			mouseLastY = glutGet(GLUT_WINDOW_HEIGHT) / 2;
		}
	}

	mouseDeltaX = (float)(mouseX - mouseLastX);
	mouseDeltaY = (float)(mouseY - mouseLastY);
	mouseLastX = mouseX;
	mouseLastY = mouseY;*/
}

void Camera::mouseMotionPassive(int x, int y)
{
	// NOTE: glutWarpPointer calls mouseMotionPassive to move the mouse
	// No mouse trigger, just update mouse X Y position
	/*if (!mouseTriggered)
	{
		mouseX = x;
		mouseY = y;
	}
	else
	{
		if (!isWrappingPointer)
		{
			mouseX = x;
			mouseY = y;

			if (mouseX != glutGet(GLUT_WINDOW_WIDTH) / 2 || mouseY != glutGet(GLUT_WINDOW_HEIGHT) / 2)
			{
				isWrappingPointer = true;
				glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);
			}
		}
		else
		{
			isWrappingPointer = false;
			mouseLastX = glutGet(GLUT_WINDOW_WIDTH) / 2;
			mouseLastY = glutGet(GLUT_WINDOW_HEIGHT) / 2;
		}
	}

	mouseDeltaX = (float)(mouseX - mouseLastX);
	mouseDeltaY = (float)(mouseY - mouseLastY);
	mouseLastX = mouseX;
	mouseLastY = mouseY;*/
}

void Camera::mouseWheel(int dir)
{

}

void Camera::keyboard(int key)
{

}

void Camera::keyboardUp(int key)
{

}

void Camera::keyboardSpecial(int key)
{

}

void Camera::keyboardSpecialUp(int key)
{

}

// Accessors definitions

void Camera::setPosition(glm::vec3 position)
{
	this->position = vec3(position.x, position.y, position.z);
	updateViewMatrix();
}

// Input: width and height of the window as int.
// Calculate aspect ratio of window, call on window reshape.
void Camera::setResolution(int width, int height)
{
	if (height == 0) height = 1;
	resolution = vec2((float)width, (float)height);
	aspectRatio = resolution.x / resolution.y;
	updateProjectionMatrix();
}

void Camera::setMaxSpeed(float maxspeed)
{
	this->maxSpeed = maxspeed;
}

void Camera::setAcceleration(float acceleration)
{
	this->acceleration = acceleration;
}

void Camera::setSmoothness(float smoothness)
{
	this->smoothness = smoothness;
}

void Camera::setMouseSensitivity(float sensitivity)
{
	this->mouseSensitivity = sensitivity;
}

void Camera::setActive(bool isActive)
{
	this->isActive = isActive;

	if (isActive)
	{
		updateProjectionMatrix();
		updateViewMatrix();
	}
}

void Camera::setFov(float angle)
{
	fov = angle;
}

void Camera::setOrtho(bool isOrtho)
{
	this->isOrtho = isOrtho;
}

mat4 Camera::getMatViewProjection() const
{
	return matProjection * matView;
}

mat4 Camera::getMatView() const
{
	return matView;
}

mat4 Camera::getMatInvView() const
{
	return matInvView;
}

mat4 Camera::getMatProjection() const
{
	return matProjection;
}

mat4 Camera::getMatInvProjection() const
{
	return matInvProjection;
}

vec2 Camera::getResolution() const
{
	return resolution;
}

vec3 Camera::getPosition() const
{
	return position;
}

glm::vec3 Camera::getDirection() const
{
	return direction;
}

float Camera::getMaxSpeed() const
{
	return maxSpeed;
}

float Camera::getAcceleration() const
{
	return acceleration;
}

float Camera::getSmoothness() const
{
	return smoothness;
}

float Camera::getMouseSensitivity() const
{
	return mouseSensitivity;
}

bool Camera::getActive() const
{
	return isActive;
}

float Camera::getNearPlane() const
{
	return zNear;
}

float Camera::getFarPlane() const
{
	return zFar;
}