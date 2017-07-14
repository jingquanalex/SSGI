#include "PointCloud.h"

PointCloud::PointCloud(GLuint colormap, GLuint positionmap)
{
	colorMap = colormap;
	positionMap = positionmap;
	pointCloudShader = new Shader("pointcloud");
	glUseProgram(pointCloudShader->getShaderId());
	glUniform1i(glGetUniformLocation(pointCloudShader->getShaderId(), "dsColor"), 0);
	glUniform1i(glGetUniformLocation(pointCloudShader->getShaderId(), "dsDepth"), 1);

	glm::ivec2* texelCoord = new glm::ivec2[texWidth * texHeight];

	int count = 0;
	for (int i = 0; i < texWidth; i++)
	{
		for (int j = 0; j < texHeight; j++)
		{
			texelCoord[count++] = glm::ivec2(i, j);
		}
	}

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, texWidth * texHeight * sizeof(glm::ivec2), texelCoord, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::ivec2), (GLvoid*)0);

	glBindVertexArray(0);
}

PointCloud::~PointCloud()
{
	delete pointCloudShader;
}

void PointCloud::draw()
{
	glUseProgram(pointCloudShader->getShaderId());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colorMap);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, positionMap);

	glBindVertexArray(vao);
	glDrawArrays(GL_POINTS, 0, texWidth * texHeight);
}

void PointCloud::drawMeshOnly()
{
	glBindVertexArray(vao);
	glDrawArrays(GL_POINTS, 0, texWidth * texHeight);
}

void PointCloud::recompileShader()
{
	pointCloudShader->recompile();
	glUseProgram(pointCloudShader->getShaderId());
	glUniform1i(glGetUniformLocation(pointCloudShader->getShaderId(), "dsColor"), 0);
	glUniform1i(glGetUniformLocation(pointCloudShader->getShaderId(), "dsDepth"), 1);
}

Shader* PointCloud::getShader()
{
	return pointCloudShader;
}