/* Engine.h: Stores declarations for the Engine class and engine-related definitions
*/

#pragma once

#include "../AppWindow.hpp"
#include <vector>
#include <unordered_set>

class Engine {
public:
	Engine(GLFWwindow *w);
	~Engine();

	// Engine
	void run();

	// Setters
		//Vulkan
	void setVulkanValidationLayers(std::vector<const char*> layers);
	
	// Getters
		// Vulkan
	inline std::vector<const char*> getEnabledVulkanValidationLayers() { return validationLayers; };
	bool verifyVulkanExtensionValidity(const char** arrayOfExtensions, uint32_t arraySize);
	bool verifyVulkanValidationLayers(std::vector<const char*> layers);

private:
	// Unclassified
	GLFWwindow* window;

	inline bool windowIsValid() const { return window != nullptr; };
	
	// Engine
	void update();
	void cleanup();
	
	// Vulkan
		// If program is not run in DEBUG mode, disable validation layers. Else, enable them.
	#ifdef NDEBUG
		const bool enableValidationLayers = false;
	#else
		const bool enableValidationLayers = true;
	#endif
	VkInstance vulkInst;
	std::vector<const char*> validationLayers;

	void initVulkan();
	VkResult createVulkanInstance();
	std::vector<VkExtensionProperties> getSupportedVulkanExtensions();
	std::vector<VkLayerProperties> getSupportedVulkanValidationLayers();

};
