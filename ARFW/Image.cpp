#include "Image.h"
/*
using namespace std;

extern std::string g_ExePath;

int loadTexture(string filename)
{
	if (filename == "") return -1;

	vector<IdNamePair>::iterator it = find_if(listTextures.begin(), listTextures.end(),
		[filename](const IdNamePair& e) { return e.second == filename; });

	if (it == listTextures.end())
	{
		cout << "Load Texture: " << filename << endl;

		int width, height, components;
		string filepath = g_ExePath + "../../" + filename;
		GLubyte* bits = stbi_load(filepath.c_str(), &width, &height, &components, 4);
		
		if (bits == 0 || width == 0 || height == 0)
		{
			cout << "Error loading texture: " << filename << endl;
			return -1;
		}

		GLuint id;
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);

		listTextures.push_back(make_pair(id, filename));

		return id;
	}
	else
	{
		return it->first;
	}
}*/