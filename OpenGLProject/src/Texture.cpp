#include "Texture.h"
#include <stb_image.h>

Texture::Texture(const std::string& filePath)
	: m_FilePath(filePath), m_LocalBuffer(nullptr), m_TextureID(0), m_Width(0), m_Height(0), m_BitsPerPixel(0) {
	// Flip texture to align with OpenGL's way of rendering images/textures
	stbi_set_flip_vertically_on_load(true);
	m_LocalBuffer = stbi_load(filePath.c_str(), &m_Width, &m_Height, &m_BitsPerPixel, 4);
	
	glGenTextures(1, &m_TextureID);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);

	// Specify texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_LocalBuffer);
	glGenerateMipmap(GL_TEXTURE_2D);
	unbind();
	
	// Local texture buffer is safe to delete after texture-binding
	if (m_LocalBuffer) {
		stbi_image_free(m_LocalBuffer);
	}
}

Texture::~Texture() {
	glDeleteTextures(1, &m_TextureID);
}

void Texture::bind(unsigned int slot) const {
	/*
		Bind texture according to its slot number
	*/
	// Get slot ID by incrementing GL_TEXTURE0 with the slot (if provided).
	// This works because GL_TEXTURE goes from 0 to 32, are contiguous (refer to GL_TEXTURE definitions) and are basically integers.
	unsigned int slotID = GL_TEXTURE0 + slot;
	glActiveTexture(slotID);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);
}

void Texture::unbind() const {
	glBindTexture(GL_TEXTURE_2D, 0);
}