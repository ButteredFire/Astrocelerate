#pragma once

#ifndef BUFFER_OBJECTS_H
#define BUFFER_OBJECTS_H

#include <GL/glew.h>

class VertexBuffer {
private:
	unsigned int m_RendererID;

public:
	VertexBuffer(const void* data, unsigned int size);
	~VertexBuffer();

	void bind() const;
	void unbind() const;
};

class IndexBuffer {
private:
	unsigned int m_RendererID;
	unsigned int m_Count;

public:
	IndexBuffer(const unsigned int* data, unsigned int count);
	~IndexBuffer();

	void bind() const;
	void unbind() const;

	inline unsigned int getCount() const { return m_Count; };
};

#endif
