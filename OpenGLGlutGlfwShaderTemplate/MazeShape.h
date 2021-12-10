#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

#include "Shape.h"
#include "Texture.h"


struct Transform
{
	glm::vec3 position{0,0,0};
	glm::vec3 scale{0,0,0};
	glm::vec3 rotation{0,0,0};
	float rotationAngle = 0;
};

class MazeShape
{
public:
	MazeShape() {}
	~MazeShape() = default;


	void setModelID(GLuint* modelId) {
		m_modelID = modelId;
	}
	void transformObject(glm::vec3 scale, glm::vec3 rotationAxis, float rotationAngle, glm::vec3 translation);
	void addShape(Shape shape, Transform transform);
	void draw(glm::vec3 position, Texture* texture);

private:
	GLuint *m_modelID = nullptr;
	std::vector<pair<Shape, Transform>> m_shape;

};
