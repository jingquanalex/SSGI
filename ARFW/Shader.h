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

	int success;
	GLuint shaderId = 0;
	std::string shaderName;

	std::string readShaderFile(std::string filename);
	GLuint compileShader(std::string name);

public:

	Shader(std::string shadername);
	~Shader();

	void apply();
	void recompile();

	bool hasError() const;
	GLuint getShaderId() const;
	std::string getShaderName() const;
	
};