/* TextureUtils.hpp - Utilities pertaining to texture manipulation.
*/

#pragma once

#include <memory>
#include <external/GLFWVulkan.hpp>

#include <imgui/imgui_impl_vulkan.h>


namespace TextureUtils {
	struct TextureProps {
		ImVec2 size;				// The texture's size.
		ImTextureID textureID;		// The texture's ID (Can be recast back to its original VkDescriptorSet form).
	};


	/* Generates an ImTextureID handle for a texture.
		@param imageLayout: The texture's format.
		@param imageView: The texture's VkImageView handle.
		@param sampler: The texture's image sampler.

		@return The texture's ImTextureID handle.
	*/
	inline ImTextureID GenerateImGuiTextureID(VkImageLayout imageLayout, VkImageView imageView, VkSampler sampler) {
		return (ImTextureID)ImGui_ImplVulkan_AddTexture(sampler, imageView, imageLayout);
	}


	/* Creates a texture for use within the ImGui GUI.

		NOTE: This function uses the texture manager. Make sure that the manager has already been registered.

		NOTE: To create a texture purely for Vulkan, please refer to the texture manager.

		@param texturePath: The path to the texture.
		@param textureFormat (Default: VK_FORMAT_R8G8B8A8_SRGB): The texture format.

		@return The texture properties.
	*/
	//inline TextureProps CreateImGuiTexture(const std::string &texturePath, VkFormat textureFormat = VK_FORMAT_R8G8B8A8_SRGB) {
	//	std::shared_ptr<TextureManager> textureManager = ServiceLocator::GetService<TextureManager>(__FUNCTION__);
	//	
	//	Geometry::Texture texture = textureManager->createIndependentTexture(texturePath, textureFormat);
	//	
	//	return TextureProps{
	//		.size = ImVec2(texture.size.x, texture.size.y),
	//		.textureID = GenerateImGuiTextureID(texture.imageLayout, texture.imageView, texture.sampler)
	//	};
	//}
}
