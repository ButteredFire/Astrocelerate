/* TextureManager.cpp - Texture manager implementation.
*/
#include "TextureManager.hpp"


void TextureManager::createTextureImage(const char* imgPath, int channels) {
	int textureWidth, textureHeight, textureChannels;
	stbi_uc* pixels = stbi_load(imgPath, &textureWidth, &textureHeight, &textureChannels, channels);

	// The size of the pixels array is equal to: width * height * bytesPerPixel
	VkDeviceSize imageSize = static_cast<uint64_t>(textureWidth * textureHeight * channels);

	if (!pixels) {
		throw Log::RuntimeException(__FUNCTION__, "Failed to create texture image for image path " + enquote(imgPath) + "!");
	}
}
