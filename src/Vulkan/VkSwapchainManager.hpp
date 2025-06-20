/* VkSwapchainManager.hpp - Manages the Vulkan swap-chain.
* 
* Handles swap-chain initialization and operations pertaining to
* swap-chains, image acquisition, and presentation.
*/

#pragma once

// GLFW & Vulkan
#include <External/GLFWVulkan.hpp>

// C++ STLs
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// Local
#include <Vulkan/VkInstanceManager.hpp>
#include <Core/Application/LoggingManager.hpp>
#include <Core/Application/GarbageCollector.hpp>
#include <Core/Engine/ServiceLocator.hpp>
#include <Core/Data/Constants.h>
#include <Core/Data/Contexts/VulkanContext.hpp>
#include <Core/Application/EventDispatcher.hpp>

#include <Vulkan/VkImageManager.hpp>


// A structure that manages properties of a swap chain.
typedef struct SwapChainProperties {
	VkSurfaceCapabilitiesKHR surfaceCapabilities{};
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;
} SwapChainProperties;


class VkSwapchainManager {
public:
	VkSwapchainManager();
	~VkSwapchainManager() = default;

	/* Initializes the swap-chain manager. */
	void init();


	/* Creates a framebuffer for each image in the swap-chain. Only call this after the graphics pipeline has been initialized. */
	void createFrameBuffers();


	/* Recreates the swap-chain. */
	void recreateSwapchain(uint32_t imageIndex);


	/* Queries the properties of a GPU's swap-chain.
		@param device: The GPU from which to query swap-chain properties.
		@param surface: The window surface to which swap-chain images are renderered.

		@return A SwapChainProperties struct containing details of the GPU's swap-chain.
	*/
	static SwapChainProperties getSwapChainProperties(VkPhysicalDevice& device, VkSurfaceKHR& surface);

private:
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<GarbageCollector> m_garbageCollector;
	std::vector<uint32_t> m_cleanupTaskIDs; // Stores cleanup task IDs (used exclusively in the swap-chain recreation process)

	VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;

	std::vector<VkImage> m_images;
	std::vector<VkImageView> m_imageViews;

	std::vector<VkFramebuffer> m_imageFrameBuffers;

	void bindEvents();

	/* Creates a swap-chain. */
	void createSwapChain();


	/* Creates an array of image views, each element corresponding to a VkImage object in the passed-in vector.
		
		BACKGROUND:
		
		- A VkImage object holds all the raw rendering data in a chunk of memory on the GPU. The data, in its raw, disorganized form, cannot be interpreted and used without some sort of wrapper or parser.
		
		- A VkImageView object is a VkImage wrapper that defines how the image should be interpreted (e.g., format, color channels, and mip levels). In Vulkan, raw images cannot be directly accessed in shaders, framebuffers, or render passes without an image view.
		
		The "VkImage + VkImageView" concept is conceptually akin to OpenGL's "Vertex Buffer + Vertex Buffer Layout".
	*/
	void createImageViews();


	/* Gets the surface format that is the most suitable for Astrocelerate.
		
		Requirements for the best surface format:
		
		- Surface format: VK_VK_FORMAT_B8G8R8A8_SRGB (8-bit RGBA)
		
		More specifically, the surface must have RGBA color channels that
		are stored in 8-bit unsigned integers each, as well as sGRB encoding.
		
		- Color space: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR (sGRB color space)
		
		More specifically, the surface must support sRGB color space.
		@param formats: A vector containing surface formats of type VkSurfaceFormatKHR to be evaluated.

		@return The best surface format in the vector, OR the first format in the vector if none satisfies both requirements.
	*/
	VkSurfaceFormatKHR getBestSurfaceFormat(std::vector<VkSurfaceFormatKHR>& formats);


	/* Gets the swap-chain presentation mode that is the most suitable for Astrocelerate.
		
		Requirement for the best presentation mode: Must support VK_PRESENT_MODE_FIFO_KHR
		
		MAILBOX_KHR works like FIFO_KHR, but instead of waiting in a queue,
		new frames replace old frames if they are ready (like triple buffering).
		This minimizes latency while preventing tearing.
		
		However, MAILBOX_KHR consumes more GPU memory, so FIFO_KHR (V-Sync)
		is set to be the fallback option, as it is guaranteed to be available on all GPUs.
		
		@param modes: A vector containing presentation modes of type VkPresentModeKHR to be evaluated.

		@return The best presentation mode in the vector, OR VK_PRESENT_MODE_FIFO_KHR if none satisfies the requirement.
	*/
	VkPresentModeKHR getBestPresentMode(std::vector<VkPresentModeKHR>& modes);


	/* Gets the best swap extent (resolution of images in the swap-chain).
		@param capabilities: A VkSurfaceCapabilitiesKHR struct from which to get the swap extent.
		
		@return The best swap extent within accepted resolution boundaries.
	*/
	VkExtent2D getBestSwapExtent(VkSurfaceCapabilitiesKHR& capabilities);
};