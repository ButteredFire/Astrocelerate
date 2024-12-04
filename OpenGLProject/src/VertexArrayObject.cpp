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

	for (unsigned int i = 0, n = bufferElements.size(); i < n; i++) {
		const auto& element = bufferElements[i];

		//logDebug(("Element [" + i + "]:\tStride: [local: " + element.stride + "]\tOffset: [element: " + element.offset + "]"));
		logDebug("Element [" << i << "]:\tStride: [local: " << element.stride << "]\tOffset: [element: " << element.offset << "]");
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, element.count, element.type, element.normalized, element.stride, (const void*)element.offset);
	}

	//
	//
}
