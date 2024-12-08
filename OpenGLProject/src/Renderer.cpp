#include "Renderer.h"

void Renderer::clear() const {
	glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::draw(const VertexArray& VAO, const IndexBuffer& IBO, const Shader& shader) const {
	VAO.bind();
	IBO.bind();
	shader.bind();

	glDrawElements(GL_TRIANGLES, IBO.getCount(), IBO.getIndexType(), nullptr);
}