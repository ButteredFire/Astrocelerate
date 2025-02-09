#pragma once

#ifndef RENDERER_H
#define RENDERER_H

#include "../buffers/VertexArrayObject.h"
#include "../buffers/BufferObjects.h"
#include "Shader.h"

class Renderer {
public:
	void clear(GLbitfield mask) const;
	void draw(const VertexArray& VAO, const IndexBuffer& IBO, const Shader& shader) const;
};

#endif