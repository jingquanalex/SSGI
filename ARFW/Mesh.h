#pragma once

#include "global.h"

struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
};

struct Texture
{
	GLuint id;
	std::string type;
	std::string path;
};

class Mesh
{

private:

	bool hasIndices;

	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;
	std::vector<Texture> textures;
	std::vector<float> posVertices;

	GLuint vbo;
	GLuint vao;
	GLuint ebo;

	void bindTexture(GLuint program);

public:

	Mesh(const std::vector<Vertex>& vertices, 
		const std::vector<GLuint>& indices, 
		const std::vector<Texture>& textures);
	~Mesh();

	void draw(GLuint program);

};