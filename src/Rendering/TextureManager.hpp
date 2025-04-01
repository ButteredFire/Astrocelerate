/* TextureManager.hpp - Manages textures and related operations (e.g., creation, modification).
*/

#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>

// STB
#include <stb/stb_image.h>

// C++ STLs
#include <iostream>

// Other
#include <Constants.h>
#include <LoggingManager.hpp>


class TextureManager {
public:
	TextureManager() {};
	~TextureManager() {};


	/* Creates a texture image.
	* @param imgPath: The path to the image.
	* @param channels (Default: STBI_rgb_alpha): The channels the texture to be created is expected to have.
	*/
	static void createTextureImage(const char* imgPath, int channels = STBI_rgb_alpha);
};