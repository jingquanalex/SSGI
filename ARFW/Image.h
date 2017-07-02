#pragma once

#include "global.h"
#include <stb_image.h>
#include <fstream>
#include <algorithm>
#include <utility>

namespace Image
{
	typedef std::pair<GLuint, std::string> IdNamePair;
	static std::vector<IdNamePair> listTextures;

	GLuint loadTexture(std::string filepath);
	GLuint loadHDRI(std::string filepath);
}