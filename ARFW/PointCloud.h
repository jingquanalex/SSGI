#pragma once

#include "global.h"
#include "Shader.h"

class PointCloud
{

private:

	GLuint vao, vbo;
	Shader* pointCloudShader;
	GLuint colorMap, positionMap;
	int texWidth = 640;
	int texHeight = 480;

public:

	PointCloud(GLuint colormap, GLuint positionmap);
	~PointCloud();

	void draw();
	void recompileShader();

	Shader* getShader();

};