#include "Quad.h"

using namespace glm;
using namespace std;

Quad::Quad(vec3 position) : Object(position)
{
	float quadVertices[30] =
	{
		-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,
		1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,
		1.0f, -1.0f,  1.0f,  1.0f,  0.0f
	};

	// Quad vbo
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid*)(3 * sizeof(float)));
	glBindVertexArray(0);
}

Quad::~Quad()
{

}

void Quad::draw()
{
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}