#pragma once

#include "global.h"
#include "Camera.h"
#include "Model.h"
#include "Shader.h"

class Object
{

protected:

	glm::mat4 matRotation;
	glm::mat4 matModel, matModelInverse;
	glm::vec3 position, rotation, scale;
	static GLuint defaultTexID;

	Model* model;
	GLuint gPassShaderId;

	GLuint lineVbo;
	GLuint lineVao;
	std::vector<glm::vec3> lineVertices;
	std::vector<BoundingBox> listBoundingBox;
	bool isBoundingBoxVisible;
	bool isVisible;
	bool isWireframeMode;

	virtual void updateModelMatrix();

	void makeBoundingBoxData();

public:

	Object(glm::vec3 position = glm::vec3());
	~Object();

	virtual void load(std::string modelname);
	virtual void update(float dt);
	virtual void draw();
	void drawMeshOnly();

	// === Accessors ===

	void setPosition(glm::vec3 position);
	void setRotation(glm::vec3 rotation);
	void setScale(glm::vec3 scale);
	void setBoundingBoxVisible(bool isVisible);
	void setVisible(bool isVisible);
	void setWireframeMode(bool isWireframe);
	void setGPassShaderId(GLuint id);

	glm::vec3 getPosition() const;
	glm::vec3 getRotation() const;
	glm::vec3 getScale() const;
	glm::mat4 getRotationMatrix() const;
	bool getBoundingBoxVisible() const;
	bool getVisible() const;
	bool getWireframeMode() const;
	const std::vector<BoundingBox>* getBoundingBoxList() const;
	static GLuint getDefaultTex();
};