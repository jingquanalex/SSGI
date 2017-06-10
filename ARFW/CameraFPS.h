#pragma once

#include "global.h"
#include "Camera.h"

class CameraFPS : public Camera
{

private:

	float yaw, pitch;
	float moveSpeed;

	bool stateLookAround = false;
	bool stateForward = false, stateBackward = false, 
		stateLeft = false, stateRight = false;

	glm::vec3 targetPoint;

public:

	CameraFPS(int windowWidth, int windowHeight);
	~CameraFPS();

	void update(float dt);

	void keyCallback(int key, int action);
	void cursorPosCallback(double x, double y);
	void mouseCallback(int button, int action);

	void setTargetPoint(glm::vec3 position);

};