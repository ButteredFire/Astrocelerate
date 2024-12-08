#pragma once

#ifndef BUFFER_OBJECTS_H
#define BUFFER_OBJECTS_H

#include "LoggingUtils.h"
#include "Constants.h"

#include <GL/glew.h>
#include <stdexcept>
#include <vector>
#include <map>

struct Vertex2D {
	float x, y;         // Coordinates (x, y)
	float r, g, b;		// Color values (RGB)
	float texX, texY;	// Texture coordinates (x, y)
};

struct VertexBufferElement {
	unsigned int location;
	unsigned int type;
	unsigned int count;
	unsigned int normalized;
	unsigned int stride;
	unsigned int offset;
};

class VertexBuffer {
private:
	unsigned int m_RendererID;

public:
	VertexBuffer(const void* data, unsigned int size);
	~VertexBuffer();

	void bind() const;
	void unbind() const;
};

class VertexBufferLayout {
private:
	std::vector<VertexBufferElement> m_BufferElements;

public:
	VertexBufferLayout() {};

	// Handle unsupported type
	template<typename T>
	void push(unsigned int location, unsigned int count, unsigned int size = sizeof(T), unsigned int offset = sizeof(T)) {
		logError(Error::UNSUPPORTED_VBO_ELEM_TYPE, "Unsupported vertex buffer element type!");
		static_assert(false);
	}

	// Template specializations
	template<>
	void push<float>(unsigned int location, unsigned int count, unsigned int size, unsigned int offset) {
		m_BufferElements.push_back({
			location, GL_FLOAT, count, GL_FALSE, size, offset
		});
	}

	template<>
	void push<unsigned int>(unsigned int location, unsigned int count, unsigned int size, unsigned int offset) {
		m_BufferElements.push_back({
			location, GL_UNSIGNED_INT, count, GL_FALSE, size, offset
		});
	}

	template<>
	void push<unsigned char>(unsigned int location, unsigned int count, unsigned int size, unsigned int offset) {
		m_BufferElements.push_back({
			location, GL_UNSIGNED_BYTE, count, GL_TRUE, size, offset
		});
	}

	template<>
	void push<Vertex2D>(unsigned int location, unsigned int count, unsigned int size, unsigned int offset) {
		m_BufferElements.push_back({
			location, GL_FLOAT, count, GL_FALSE, size, offset
		});
	}

	inline std::vector<VertexBufferElement> getVertexBufferElements() const { return m_BufferElements; }
};

class IndexBuffer {
private:
	unsigned int m_RendererID;
	unsigned int m_Count;
	unsigned int m_IndexType;

public:
	IndexBuffer(const unsigned int* data, unsigned int count, unsigned int indexType);
	~IndexBuffer();

	void bind() const;
	void unbind() const;

	inline unsigned int getCount() const { return m_Count; };
	inline unsigned int getIndexType() const { return m_IndexType; };
};

#endif
