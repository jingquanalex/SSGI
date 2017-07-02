#include "Model.h"

using namespace glm;
using namespace std;
using namespace Assimp;

Model::Model(string filepath)
{
	loadModel(filepath);
}

Model::~Model()
{

}

void Model::draw(GLuint program)
{
	for (Mesh& mesh : meshes)
	{
		mesh.draw(program);
	}
}

void Model::drawMeshOnly()
{
	for (Mesh& mesh : meshes)
	{
		mesh.drawMeshOnly();
	}
}

void Model::loadModel(string path)
{
	Importer import;
	cout << "Loading model: " << path << endl;
	const aiScene* scene = import.ReadFile(path, aiProcess_GenNormals | aiProcess_Triangulate | aiProcess_FlipUVs); //aiProcess_Triangulate | aiProcess_FlipUVs

	if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		cout << "ERROR::ASSIMP::" << import.GetErrorString() << endl;
		return;
	}

	directory = path.substr(0, path.find_last_of('/'));
	processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
	// Process all the node's meshes (if any)
	for (GLuint i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh, scene));
	}

	// Then do the same for each of its children
	for (GLuint i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene);
	}
}

// Extract revelent data from assimp mesh for our dataset
Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
	vector<Vertex> vertices;
	vector<GLuint> indices;
	vector<Texture> textures;

	// Process vertices
	for (GLuint i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;

		vertex.Position = vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
		vertex.Normal = vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
		if (mesh->mTextureCoords[0])
		{
			vertex.TexCoords = vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
		}
		else
		{
			vertex.TexCoords = vec2();
		}

		// Make bounding box
		bbox.Min.x = vertex.Position.x < bbox.Min.x ? vertex.Position.x : bbox.Min.x;
		bbox.Max.x = vertex.Position.x > bbox.Max.x ? vertex.Position.x : bbox.Max.x;
		bbox.Min.y = vertex.Position.y < bbox.Min.y ? vertex.Position.y : bbox.Min.y;
		bbox.Max.y = vertex.Position.y > bbox.Max.y ? vertex.Position.y : bbox.Max.y;
		bbox.Min.z = vertex.Position.z < bbox.Min.z ? vertex.Position.z : bbox.Min.z;
		bbox.Max.z = vertex.Position.z > bbox.Max.z ? vertex.Position.z : bbox.Max.z;

		vertices.push_back(vertex);
	}
	
	// Process indices
	for (GLuint i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (GLuint j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	// Process material
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

		vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "normal");
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

		//vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "specular");
		//textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
	}

	return Mesh(vertices, indices, textures);
}

vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
{
	vector<Texture> textures;
	vector<Texture> textures_loaded;

	for (GLuint i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		GLboolean skip = false;

		for (GLuint j = 0; j < textures_loaded.size(); j++)
		{
			if (textures_loaded[j].path == str.C_Str())
			{
				textures.push_back(textures_loaded[j]);
				skip = true;
				break;
			}
		}

		if (!skip)
		{
			// If texture hasn't been loaded already, load it
			Texture texture;
			texture.id = TextureFromFile(str.C_Str());
			texture.type = typeName;
			texture.path = str.C_Str();
			textures.push_back(texture);
			textures_loaded.push_back(texture);
		}
	}

	return textures;
}

GLint Model::TextureFromFile(const char* path)
{
	// Generate texture ID and load texture data 
	string filepath = string(path);
	filepath = directory + '/' + filepath;
	return Image::loadTexture(filepath);
}

BoundingBox Model::getBoundingBox() const
{
	return bbox;
}