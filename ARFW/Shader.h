#pragma once

#include "global.h"
#include <fstream>
#include <algorithm>
#include <utility>

class Shader
{

private:

	typedef std::pair<GLuint, std::string> IdNamePair;
	static std::vector<IdNamePair> shaderList;

	GLuint shaderId;
	std::string shaderName;

	std::string readShaderFile(std::string filename);
	GLuint compileShader(std::string name);

public:

	Shader(std::string shadername);
	~Shader();

	void apply();

	GLuint getShaderId() const;
	std::string getShaderName() const;

};