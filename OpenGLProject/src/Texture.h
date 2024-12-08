#pragma once

#ifndef TEXTURE_H
#define TEXTURE_H

#include "Renderer.h"

class Texture {
private:
	unsigned int m_TextureID;
	std::string m_FilePath;
	unsigned char* m_LocalBuffer;
	int m_Width, m_Height, m_BitsPerPixel;

public:
	Texture(const std::string& filePath);
	~Texture();

	void bind(unsigned int slot = 0) const;
	void unbind() const;

	inline int getWidth() const { return m_Width; };
	inline int getHeight() const { return m_Height; };
};

#endif