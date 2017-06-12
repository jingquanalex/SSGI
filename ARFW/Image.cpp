#define STB_IMAGE_IMPLEMENTATION
#include "Image.h"

using namespace std;
using namespace Image;

unsigned int Image::loadTexture(string filepath)
{
	if (filepath == "") return -1;

	vector<IdNamePair>::iterator it = find_if(listTextures.begin(), listTextures.end(),
		[filepath](const IdNamePair& e) { return e.second == filepath; });

	if (it == listTextures.end())
	{
		cout << "Texture Loaded: " << filepath << endl;

		int width, height, channels;
		GLubyte* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
		
		if (data == 0 || width == 0 || height == 0)
		{
			cout << "Error loading texture: " << filepath << endl;
			stbi_image_free(data);
			return -1;
		}

		GLuint id;
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);

		GLenum format;
		if (channels == 1)
			format = GL_RED;
		else if (channels == 3)
			format = GL_RGB;
		else if (channels == 4)
			format = GL_RGBA;

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glGenerateMipmap(GL_TEXTURE_2D);
		stbi_image_free(data);

		listTextures.push_back(make_pair(id, filepath));

		return id;
	}
	else
	{
		return it->first;
	}
}