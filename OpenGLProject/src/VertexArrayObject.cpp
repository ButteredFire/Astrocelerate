#include "VertexArrayObject.h"

#include <iostream>
#include <GL/glew.h>

VertexArray::VertexArray() {
	glGenVertexArrays(1, &m_RendererID);
	bind();
}

VertexArray::~VertexArray() {
	glDeleteVertexArrays(1, &m_RendererID);
}

void VertexArray::bind() const {
	glBindVertexArray(m_RendererID);
}

void VertexArray::unbind() const {
	glBindVertexArray(0);
}

void VertexArray::addBuffer(const VertexBuffer& VBO, const VertexBufferLayout& VBLayout) {
	bind();
	VBO.bind();
	
	const auto& bufferElements = VBLayout.getVertexBufferElements();
	unsigned int offset = 0;

	for (unsigned int i = 0, n = bufferElements.size(); i < n; i++) {
		const auto& element = bufferElements[i];

		//std::cout << "Stride: [local: " << VBLayout.getStride() << "; sizeof: " << sizeof(Vertex2D) << "]\tOffset: [local: " << offset << "; element: " << element.offset << "; actual: " << offsetof(Vertex2D, x) << "]\n";
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, element.count, element.type, element.normalized, VBLayout.getStride(), (const void*) offset);

		offset += element.count * element.offset;
	}
}
