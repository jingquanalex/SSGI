#include "Object.h"

using namespace glm;
using namespace std;

extern string g_ExePath;

GLuint Object::defaultTexID;

Object::Object(vec3 position)
{
	model = nullptr;

	this->position = position;
	this->rotation = vec3(0);
	this->scale = vec3(1);
	this->up = vec3(0, 1, 0);

	matTranslate = mat4(1);
	matRotation = mat4(1);
	matScale = mat4(1);

	updateModelMatrix();
	
	boundingBoxVisible = false;
	visible = true;
	wireframe = false;
}

Object::~Object()
{

}

void Object::load(string filepath)
{
	model = new Model(g_ExePath + "../../media/" + filepath);
	boundingBox = model->getBoundingBox();
	
	glGenVertexArrays(1, &lineVao);
	glGenBuffers(1, &lineVbo);

	glBindVertexArray(lineVao);
	glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
	glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(vec3), NULL, GL_STREAM_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glBindVertexArray(0);

	updateBoundingBoxData();
}

void Object::update(float dt)
{

}

void Object::draw()
{
	if (model != nullptr && visible)
	{
		if (wireframe)
		{
			glDisable(GL_CULL_FACE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}

		glUniformMatrix4fv(glGetUniformLocation(gPassShaderId, "model"), 1, GL_FALSE, value_ptr(matModel));
		glUniformMatrix4fv(glGetUniformLocation(gPassShaderId, "modelInverse"), 1, GL_FALSE, value_ptr(matModelInverse));
		model->draw(gPassShaderId);

		glEnable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (!lineVertices.empty() && boundingBoxVisible)
	{
		glBindVertexArray(lineVao);
		glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)lineVertices.size());
	}
}

void Object::drawMeshOnly()
{
	glUniformMatrix4fv(glGetUniformLocation(gPassShaderId, "model"), 1, GL_FALSE, value_ptr(matModel));
	glUniformMatrix4fv(glGetUniformLocation(gPassShaderId, "modelInverse"), 1, GL_FALSE, value_ptr(matModelInverse));
	model->drawMeshOnly();

	/*if (!lineVertices.empty() && boundingBoxVisible) // TODO: crash??
	{
		glBindVertexArray(lineVao);
		glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)lineVertices.size());
	}*/
}

void Object::updateModelMatrix()
{
	matModel = matTranslate * matRotation * matScale;
	matModelInverse = inverse(matModel);
	updateBoundingBoxData();
}

// Update bounding box VBO
// Make line strip vertices, also store the transformed vertices and face normals
void Object::updateBoundingBoxData()
{
	lineVertices.clear();

	BoundingBox* bb = &boundingBox;

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

	bb->Height = this->scale.y * (bb->Max.y - bb->Min.y);

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

	// Remove duplicate normals
	/*sort(boundingBoxSATNormals.begin(), boundingBoxSATNormals.end(), 
		[](const vec3& v1, const vec3& v2){ return v1.x < v2.x; });
	boundingBoxSATNormals.erase(
		unique(boundingBoxSATNormals.begin(), boundingBoxSATNormals.end()), 
		boundingBoxSATNormals.end());*/

	if (!lineVertices.empty())
	{
		glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, lineVertices.size() * sizeof(vec3), &lineVertices[0]);
	}
}

void Object::setPosition(vec3 position)
{
	this->position = position;
	matTranslate = glm::translate(this->position);
	updateModelMatrix();
}

void Object::setRotationEulerAngle(vec3 rotation)
{
	this->rotation = vec3(radians(rotation.x), radians(rotation.y), radians(rotation.z));
	matRotation = eulerAngleXYZ(this->rotation.x, this->rotation.y, this->rotation.z);
	updateModelMatrix();
}

void Object::setUpVector(glm::vec3 up)
{
	this->up = normalize(up);
}

void Object::setRotationByAxisAngle(glm::vec3 axis, float angle)
{
	matRotation = orientation(normalize(axis), up) * rotate(radians(angle), up);
	updateModelMatrix();
}

void Object::setScale(vec3 scale)
{
	this->scale = scale;
	matScale = glm::scale(this->scale);
	updateModelMatrix();
}

void Object::setBoundingBoxVisible(bool isVisible)
{
	this->boundingBoxVisible = isVisible;
}

void Object::setVisible(bool isVisible)
{
	this->visible = isVisible;
}

void Object::setWireframeMode(bool isWireframe)
{
	this->wireframe = isWireframe;
}

void Object::setGPassShaderId(GLuint id)
{
	this->gPassShaderId = id;
}

vec3 Object::getPosition() const
{
	return position;
}

vec3 Object::getRotationEulerAngle() const
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

BoundingBox Object::getBoundingBox() const
{
	return boundingBox;
}

bool Object::isBoundingBoxVisible() const
{
	return boundingBoxVisible;
}

bool Object::isVisible() const
{
	return visible;
}

bool Object::isWireframe() const
{
	return wireframe;
}

GLuint Object::getDefaultTex()
{
	return defaultTexID;
}