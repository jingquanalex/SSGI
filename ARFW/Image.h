#pragma once

#include "global.h"
#include <stb_image.h>
#include <fstream>
#include <algorithm>
#include <utility>

typedef std::pair<GLuint, std::string> IdNamePair;
static std::vector<IdNamePair> listTextures;
unsigned int loadTexture(std::string filepath);
