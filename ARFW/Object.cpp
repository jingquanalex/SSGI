#include "Object.h"

using namespace glm;
using namespace std;

extern string g_ExePath;

GLuint Object::defaultTexID;

Object::Object(vec3 position)
{
	model = nullptr;
	shader = nullptr;

	this->position = position;
	this->rotation = vec3(0.0);
	this->scale = vec3(1.0);

	updateModelMatrix();
	matNormal = mat4();
	matRotation = mat4();

	isBoundingBoxVisible = false;
	isVisible = true;
	isWireframeMode = false;
}

Object::~Object()
{

}

void Object::load(string modelname, string shadername)
{
	model = new Model(modelname);
	shader = new Shader(shadername);

	// Create default white diffuse texture
	if (defaultTexID == NULL)
	{
		//TODO
		/*defaultTexID = SOIL_load_OGL_texture((g_ExePath + "grass.png").c_str(), SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);*/

		glBindTexture(GL_TEXTURE_2D, defaultTexID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	listBoundingBox.push_back(model->getBoundingBox());
	makeBoundingBoxData();

	glGenVertexArrays(1, &lineVao);
	glGenBuffers(1, &lineVbo);

	glBindVertexArray(lineVao);
	glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
	glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(vec3), NULL, GL_STREAM_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glBindVertexArray(0);
}

void Object::update(float dt)
{
	// Update bounding box VBO
	if (!lineVertices.empty())
	{
		makeBoundingBoxData();
		glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, lineVertices.size() * sizeof(vec3), &lineVertices[0]);
	}
}

void Object::draw()
{
	if (shader != nullptr)
	{
		GLuint program = shader->getShaderId();
		glUseProgram(program);
		glUniformMatrix4fv(10, 1, GL_FALSE, value_ptr(matModel));
		glUniformMatrix4fv(11, 1, GL_FALSE, value_ptr(matNormal));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, defaultTexID);

		if (model != nullptr && isVisible)
		{
			if (isWireframeMode)
			{
				glDisable(GL_CULL_FACE);
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			}

			model->draw(program);

			glEnable(GL_CULL_FACE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		if (!lineVertices.empty() && isBoundingBoxVisible)
		{
			glUniformMatrix4fv(10, 1, GL_FALSE, value_ptr(mat4()));
			glBindVertexArray(lineVao);
			glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)lineVertices.size());
			glBindVertexArray(0);
		}
	}
}

void Object::updateModelMatrix()
{
	matRotation = eulerAngleXYZ(rotation.x, rotation.y, rotation.z);
	matModel = translate(position) * glm::scale(scale) * matRotation;
}

void Object::updateNormalMatrix()
{
	matNormal = transpose(inverse(matModel));
}

// Make line strip vertices, also store the transformed vertices and face normals
void Object::makeBoundingBoxData()
{
	lineVertices.clear();

	for (auto bb = listBoundingBox.begin(); bb != listBoundingBox.end(); bb++)
	{
		// Bottom verts
		vec3 v1 = vec3(matModel * vec4(bb->Min.x, bb->Min.y, bb->Max.z, 1));
		vec3 v2 = vec3(matModel * vec4(bb->Max.x, bb->Min.y, bb->Max.z, 1));
		vec3 v3 = vec3(matModel * vec4(bb->Max.x, bb->Min.y, bb->Min.z, 1));
		vec3 v4 = vec3(matModel * vec4(bb->Min.x, bb->Min.y, bb->Min.z, 1));
		// Top verts
		vec3 v5 = vec3(matModel * vec4(bb->Min.x, bb->Max.y, bb->Max.z, 1));
		vec3 v6 = vec3(matModel * vec4(bb->Max.x, bb->Max.y, bb->Max.z, 1));
		vec3 v7 = vec3(matModel * vec4(bb->Max.x, bb->Max.y, bb->Min.z, 1));
		vec3 v8 = vec3(matModel * vec4(bb->Min.x, bb->Max.y, bb->Min.z, 1));

		lineVertices.push_back(v1);
		lineVertices.push_back(v2);
		lineVertices.push_back(v3);
		lineVertices.push_back(v4);
		lineVertices.push_back(v1);
		lineVertices.push_back(v5);
		lineVertices.push_back(v6);
		lineVertices.push_back(v2);
		lineVertices.push_back(v6);
		lineVertices.push_back(v7);
		lineVertices.push_back(v3);
		lineVertices.push_back(v7);
		lineVertices.push_back(v8);
		lineVertices.push_back(v4);
		lineVertices.push_back(v8);
		lineVertices.push_back(v5);
		lineVertices.push_back(v1);

		bb->AxisX = vec3(matModel * vec4(bb->AxisX, 0));
		bb->AxisY = vec3(matModel * vec4(bb->AxisY, 0));
		bb->AxisZ = vec3(matModel * vec4(bb->AxisZ, 0));

		// Store vertices of bounding box
		bb->Vertices.clear();
		bb->Vertices.push_back(v1);
		bb->Vertices.push_back(v2);
		bb->Vertices.push_back(v3);
		bb->Vertices.push_back(v4);
		bb->Vertices.push_back(v5);
		bb->Vertices.push_back(v6);
		bb->Vertices.push_back(v7);
		bb->Vertices.push_back(v8);

		// Store face normals for seperating axis test
		bb->SATNormals.clear();
		bb->SATNormals.push_back(normalize(v3 - v4)); // X
		bb->SATNormals.push_back(normalize(v8 - v4)); // Y
		bb->SATNormals.push_back(normalize(v1 - v4)); // Z
	}

	// Remove duplicate normals
	/*sort(boundingBoxSATNormals.begin(), boundingBoxSATNormals.end(), 
		[](const vec3& v1, const vec3& v2){ return v1.x < v2.x; });
	boundingBoxSATNormals.erase(
		unique(boundingBoxSATNormals.begin(), boundingBoxSATNormals.end()), 
		boundingBoxSATNormals.end());*/

}

void Object::setPosition(vec3 position)
{
	this->position = position;
	updateModelMatrix();
}

void Object::setRotation(vec3 rotation)
{
	this->rotation = vec3(radians(rotation.x), radians(rotation.y), radians(rotation.z));
	updateModelMatrix();
	updateNormalMatrix();
}

void Object::setScale(vec3 scale)
{
	this->scale = scale;
	updateModelMatrix();
	updateNormalMatrix();
}

void Object::setBoundingBoxVisible(bool isVisible)
{
	this->isBoundingBoxVisible = isVisible;
}

void Object::setVisible(bool isVisible)
{
	this->isVisible = isVisible;
}

void Object::setWireframeMode(bool isWireframe)
{
	this->isWireframeMode = isWireframe;
}

vec3 Object::getPosition() const
{
	return position;
}

vec3 Object::getRotation() const
{
	return rotation;
}

vec3 Object::getScale() const
{
	return scale;
}

mat4 Object::getRotationMatrix() const
{
	return matRotation;
}

Shader* Object::getShader() const
{
	return shader;
}

bool Object::getBoundingBoxVisible() const
{
	return isBoundingBoxVisible;
}

bool Object::getVisible() const
{
	return isVisible;
}

bool Object::getWireframeMode() const
{
	return isWireframeMode;
}

const vector<BoundingBox>* Object::getBoundingBoxList() const
{
	return &listBoundingBox;
}

GLuint Object::getDefaultTex()
{
	return defaultTexID;
}