#include "Renderer.h"

void Renderer::clear(GLbitfield mask) const {
	glClear(mask);
}

void Renderer::draw(const VertexArray& VAO, const IndexBuffer& IBO, const Shader& shader) const {
	VAO.bind();
	IBO.bind();
	shader.bind();

	glDrawElements(GL_TRIANGLES, IBO.getCount(), IBO.getIndexType(), nullptr);
}