#pragma once

#ifndef RENDERER_H
#define RENDERER_H

#include "VertexArrayObject.h"
#include "BufferObjects.h"
#include "Shader.h"

class Renderer {
public:
	void clear() const;
	void draw(const VertexArray& VAO, const IndexBuffer& IBO, const Shader& shader) const;
};

#endif