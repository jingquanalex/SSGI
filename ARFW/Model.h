#pragma once

#include "global.h"
#include "Mesh.h"
#include "Image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct BoundingBox
{
	BoundingBox() { }
	BoundingBox(glm::vec3 min, glm::vec3 max) : Min(min), Max(max)
	{
		AxisX = glm::vec3(1, 0, 0);
		AxisY = glm::vec3(0, 1, 0);
		AxisZ = glm::vec3(0, 0, 1);
		Width = abs(Max.x - Min.x);
		Height = abs(Max.y - Min.y);
		Depth = abs(Max.z - Min.z);
	}
	glm::vec3 Min, Max;
	glm::vec3 AxisX, AxisY, AxisZ;
	float Width, Height, Depth;
	std::vector<glm::vec3> Vertices;
	std::vector<glm::vec3> SATNormals;
};

class Model
{

private:

	BoundingBox bbox;
	std::vector<Mesh> meshes;
	std::string directory;

	void loadModel(std::string path);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string name);
	GLint TextureFromFile(const char* path);

public:

	Model(std::string filename);
	~Model();

	void draw(GLuint program);

	BoundingBox getBoundingBox() const;

};