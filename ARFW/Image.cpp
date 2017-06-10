#define STB_IMAGE_IMPLEMENTATION
#include "Image.h"

using namespace std;

int loadTexture(string filepath)
{
	if (filepath == "") return -1;

	vector<IdNamePair>::iterator it = find_if(listTextures.begin(), listTextures.end(),
		[filepath](const IdNamePair& e) { return e.second == filepath; });

	if (it == listTextures.end())
	{
		cout << "Bitmap Loaded: " << filepath << endl;

		int width, height, components;
		GLubyte* bits = stbi_load(filepath.c_str(), &width, &height, &components, 4);
		
		if (bits == 0 || width == 0 || height == 0)
		{
			cout << "Error loading texture: " << filepath << endl;
			return -1;
		}

		GLuint id;
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);

		listTextures.push_back(make_pair(id, filepath));

		return id;
	}
	else
	{
		return it->first;
	}
}