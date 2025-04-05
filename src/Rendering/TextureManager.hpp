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
#include <ApplicationContext.hpp>
#include <MemoryManager.hpp>
#include <Shaders/BufferManager.hpp>


class TextureManager {
public:
	TextureManager() {};
	~TextureManager() {};


	/* Creates a texture image.
		@param vkContext: The application context.
		@param memoryManager: An instance of the memory manager. This is used to create a cleanup task after buffer creation.
		@param imgPath: The path to the image.
		@param channels (Default: STBI_rgb_alpha): The channels the texture to be created is expected to have.
	*/
	static void createTextureImage(VulkanContext& vkContext, MemoryManager& memoryManager, const char* imgPath, int channels = STBI_rgb_alpha);
};