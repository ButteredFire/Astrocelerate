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

	for (const auto& element : bufferElements) {
		//const auto& element = bufferElements[i];

		//logDebug("Element [" << element.location << "]:\tStride: [local: " << element.stride << "]\tOffset: [element: " << element.offset << "]");
		glEnableVertexAttribArray(element.location);
		glVertexAttribPointer(element.location, element.count, element.type, element.normalized, element.stride, (const void*)element.offset);
	}
}
