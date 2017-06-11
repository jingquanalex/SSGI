#pragma once

#include "global.h"
#include "Object.h"

class Quad : public Object
{

private:

	GLuint vbo;
	GLuint vao;

public:

	Quad(glm::vec3 position = glm::vec3());
	~Quad();

	void draw();

};