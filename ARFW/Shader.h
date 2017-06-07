#pragma once

#include "global.h"
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <utility>

class Shader
{

private:

	typedef std::pair<GLuint, std::string> NameId;
	static std::vector<NameId> shaderList;

	GLuint shaderId;
	std::string shaderName;

	std::string readShaderFile(std::string filename);
	GLuint compileShader(std::string name);

public:

	Shader(std::string shadername);
	~Shader();

	GLuint getShaderId() const;
	std::string getShaderName() const;

};