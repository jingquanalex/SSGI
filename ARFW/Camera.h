#pragma once

#include "global.h"

// Camera class that builds the projection and view matrices.
// Base class to build specialized cameras.
class Camera
{

protected:

	bool isWrappingPointer;
	bool mouseTriggered;
	bool isActive;
	bool isOrtho;

	glm::mat4 matProjection, matView;
	glm::mat4 matInvProjection, matInvView;
	glm::vec3 position, direction, up;
	float aspectRatio, fov, zNear, zFar;
	glm::vec2 resolution;

	int mouseX, mouseY, mouseLastX, mouseLastY;
	float mouseDeltaX, mouseDeltaY;
	float maxSpeed, acceleration, smoothness;
	float mouseSensitivity;

	virtual void updateProjectionMatrix();
	virtual void updateViewMatrix();

public:

	Camera(int windowWidth, int windowHeight);
	~Camera();

	// === Functions ===

	virtual void update(float dt);
	virtual void mouse(int button, int state);
	virtual void mouseMotion(int x, int y);
	virtual void mouseMotionPassive(int x, int y);
	virtual void mouseWheel(int dir);
	virtual void keyboard(int key);
	virtual void keyboardUp(int key);
	virtual void keyboardSpecial(int key);
	virtual void keyboardSpecialUp(int key);

	// === Accessors ===

	void setPosition(glm::vec3 position);
	void setResolution(int width, int height);
	void setMaxSpeed(float maxspeed);
	void setAcceleration(float acceleration);
	void setSmoothness(float smoothness);
	void setMouseSensitivity(float sensitivity);
	void setActive(bool isActive);
	void setFov(float angle);
	void setOrtho(bool isOrtho);
	
	glm::mat4 getMatViewProjection() const;
	glm::mat4 getMatView() const;
	glm::mat4 getMatInvView() const;
	glm::mat4 getMatProjection() const;
	glm::mat4 getMatInvProjection() const;
	glm::vec2 getResolution() const;
	glm::vec3 getPosition() const;
	glm::vec3 getDirection() const;
	float getMaxSpeed() const;
	float getAcceleration() const;
	float getSmoothness() const;
	float getMouseSensitivity() const;
	bool getActive() const;
	float getNearPlane() const;
	float getFarPlane() const;

};