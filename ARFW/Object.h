#pragma once

#include "global.h"
#include "Camera.h"
#include "Model.h"
#include "Shader.h"

class Object
{

protected:

	glm::vec3 position, rotation, scale, up;
	glm::mat4 matTranslate, matRotation, matScale;
	glm::mat4 matModel, matModelInverse;
	static GLuint defaultTexID;

	Model* model;
	BoundingBox boundingBox;
	GLuint gPassShaderId;

	GLuint lineVbo;
	GLuint lineVao;
	std::vector<glm::vec3> lineVertices;
	bool boundingBoxVisible;
	bool visible;
	bool wireframe;

	virtual void updateModelMatrix();
	void updateBoundingBoxData();

public:

	Object(glm::vec3 position = glm::vec3());
	~Object();

	virtual void load(std::string modelname);
	virtual void update(float dt);
	virtual void draw();
	void drawMeshOnly();

	// === Accessors ===

	void setPosition(glm::vec3 position);
	void setRotationEulerAngle(glm::vec3 rotation);
	void setUpVector(glm::vec3 up);
	void setRotationByAxisAngle(glm::vec3 axis, float angle);
	void setScale(glm::vec3 scale);
	void setBoundingBoxVisible(bool isVisible);
	void setVisible(bool isVisible);
	void setWireframeMode(bool isWireframe);
	void setGPassShaderId(GLuint id);

	glm::vec3 getPosition() const;
	glm::vec3 getRotationEulerAngle() const;
	glm::vec3 getScale() const;
	glm::mat4 getRotationMatrix() const;
	BoundingBox getBoundingBox() const;
	bool isBoundingBoxVisible() const;
	bool isVisible() const;
	bool isWireframe() const;
	static GLuint getDefaultTex();
};