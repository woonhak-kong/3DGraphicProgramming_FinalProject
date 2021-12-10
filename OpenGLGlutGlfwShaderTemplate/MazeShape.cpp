#include "MazeShape.h"

#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>

void MazeShape::transformObject(glm::vec3 scale, glm::vec3 rotationAxis, float rotationAngle, glm::vec3 translation)
{
	glm::mat4 Model;
	Model = glm::mat4(1.0f);
	Model = glm::translate(Model, translation);
	Model = glm::rotate(Model, glm::radians(rotationAngle), rotationAxis);
	Model = glm::scale(Model, scale);

	// We must now update the View.
	//calculateView();

	glUniformMatrix4fv(*m_modelID, 1, GL_FALSE, &Model[0][0]);
}

void MazeShape::addShape(Shape shape, Transform transform)
{
	shape.BufferShape();
	m_shape.push_back(pair<Shape, Transform>(shape, transform));

}

void MazeShape::draw(glm::vec3 position, Texture* texture)
{
	if (m_modelID == nullptr)
	{
		std::cout << "ModelID is empty!!! " << std::endl;
		return;
	}
	for (int i = 0; i < m_shape.size(); i++)
	{
		texture->Bind(GL_TEXTURE0);
		m_shape[i].first.RecolorShape(1.0f, 1.0f, 1.0f);
		transformObject(m_shape[i].second.scale, m_shape[i].second.rotation, m_shape[i].second.rotationAngle
			, { m_shape[i].second.position.x + position.x , m_shape[i].second.position.y + position.y, m_shape[i].second.position.z + position.z });
		m_shape[i].first.DrawShape(GL_TRIANGLES);
		glBindTexture(GL_TEXTURE_2D, 0);
	}


}
