#pragma once

#ifndef BUFFER_OBJECTS_H
#define BUFFER_OBJECTS_H

#include <GL/glew.h>
#include <stdexcept>
#include <vector>
#include <map>

struct Vertex2D {
	float x, y;        // Positions (x, y)
	//float r, g, b;  // Color values (RGB)
};

struct VertexBufferElement {
	unsigned int type;
	unsigned int count;
	unsigned int normalized;
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
	unsigned int m_Stride;

public:
	VertexBufferLayout() : m_Stride(0) {};

	// Handle unsupported type
	template<typename T>
	void push(unsigned int count, unsigned int size = sizeof(T), unsigned int offset = sizeof(T)) {
		static_assert("[RTE]: Unsupported vertex buffer element type.");
	}

	// Template specializations
	template<>
	void push<float>(unsigned int count, unsigned int size, unsigned int offset) {
		m_BufferElements.push_back({
			GL_FLOAT, count, GL_FALSE, offset	
		});
		m_Stride += size;
	}

	template<>
	void push<unsigned int>(unsigned int count, unsigned int size, unsigned int offset) {
		m_BufferElements.push_back({
			GL_UNSIGNED_INT, count, GL_FALSE, offset
		});
		m_Stride += size;
	}

	template<>
	void push<unsigned char>(unsigned int count, unsigned int size, unsigned int offset) {
		m_BufferElements.push_back({
			GL_UNSIGNED_BYTE, count, GL_TRUE, offset
		});
		m_Stride += size;
	}

	template<>
	void push<Vertex2D>(unsigned int count, unsigned int size, unsigned int offset) {
		m_BufferElements.push_back({
			GL_FLOAT, count, GL_FALSE, offset
		});
		m_Stride += size;
	}

	inline std::vector<VertexBufferElement> getVertexBufferElements() const { return m_BufferElements; }
	inline unsigned int getStride() const { return m_Stride; }
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
