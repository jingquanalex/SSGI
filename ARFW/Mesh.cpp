#include "Mesh.h"

using namespace glm;
using namespace std;

Mesh::Mesh(const vector<Vertex>& vertices, const vector<GLuint>& indices, const vector<Texture>& textures)
{
	this->hasIndices = true;
	this->vertices = vertices;
	this->indices = indices;
	this->textures = textures;

	// Make buffers and setup data formats
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
		(GLvoid*)0);
	
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
		(GLvoid*)offsetof(Vertex, Vertex::Normal));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
		(GLvoid*)offsetof(Vertex, Vertex::TexCoords));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);
	glBindVertexArray(0);
}

Mesh::~Mesh()
{

}

void Mesh::draw(GLuint program)
{
	if (!textures.empty() && program != NULL) bindTexture(program);
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Mesh::bindTexture(GLuint program)
{
	GLuint diffuseIndex = 1, specularIndex = 1, normalIndex = 1;
	for (GLuint i = 0; i < textures.size(); i++)
	{
		stringstream ss;
		string number;
		string name = textures[i].type;
		if (name == "diffuse")
			ss << diffuseIndex++;
		else if (name == "normal")
			ss << normalIndex++;
		else if (name == "specular")
			ss << specularIndex++;
		number = ss.str();

		glActiveTexture(GL_TEXTURE0 + i);
		glUniform1i(glGetUniformLocation(program, (name + number).c_str()), i);
		glBindTexture(GL_TEXTURE_2D, textures[i].id);
	}
}