/* VkPipelineManager.hpp - Manages pipelines pertaining to graphics (e.g., graphics pipeline, compute pipeline).
* 
* Handles multiple graphics pipelines and related operations (e.g., pipeline creation, destruction, caching).
* Stores pipeline layouts, render passes, and pipeline objects.
*/
#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// C++ STLs
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// Local
#include <Vulkan/VkInstanceManager.hpp>
#include <LoggingManager.hpp>
#include <Constants.h>
#include <VulkanContexts.hpp>


/* Reads a file in binary mode.
* @param fileName: The name of the file to be read.
* @param workingDirectory (optional): The path to the file. By default, it is set to the current working directory (according to std::filesystem::current_path()).
* @param defaultWorkingDirectory (optional): The path to which the working directory will reset after reading the file. In case workingDirectory is specified, the current path (i.e., std::filesystem::current_path()) is changed to it. Therefore, if you have set the working directory to something other than the default value (at compilation time) prior to invoking this function, you must specify defaultWorkingDirectory to it.
* @return A byte array (vector of type char) containing the file's content.
*/
static inline std::vector<char> readFile(const std::string& fileName, const std::string& workingDirectory = std::filesystem::current_path().string(), const std::string& defaultWorkingDirectory = DEFAULT_WORKING_DIR) {
	// Changes the current working directory to the specified one (if provided)
	std::filesystem::path workingDirPath = workingDirectory; // Converts string to filesystem::path
	std::filesystem::current_path(workingDirPath);
	
	// Reads the file to an ifstream
	// Flag std::ios::ate means "Start reading from the end of the file". Doing so allows us to use the read position to determine the file size and allocate a buffer.
	// Flag std::ios::binary means "Read the file as a binary file (to avoid text transformations)"
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	// If file cannot open
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file " + enquote(fileName) + "!"
			+ "\n" + "The file may not be in the correct working directory of " + enquote(workingDirectory)
		);
	}

	// Allocates a buffer using the current reading position
	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	// Seeks back to the beginning of the file and reads all of the bytes at once
	file.seekg(0);
	file.read(buffer.data(), fileSize);

	// If file is incomplete/not read successfully
	if (!file) {
		throw std::runtime_error("Failed to read file " + enquote(fileName) + "!");
	}

	file.close();

	// Revert working directory to the default/current one
	std::filesystem::path defaultDirPath = defaultWorkingDirectory;
	std::filesystem::current_path(defaultDirPath);

	return buffer;
}


class GraphicsPipeline {
public:
	GraphicsPipeline(VulkanContext& context);
	~GraphicsPipeline();

	void init();

private:
	VulkanContext& vkContext;

	std::vector<char> vertShaderBytecode;
	std::vector<char> fragShaderBytecode;

	/* Loads compiled SPIR-V shader files. */
	void loadShaders();


	/* Creates a shader module to pass the code to the pipeline.
	* @param bytecode: The SPIR-V shader file bytecode data.
	* @return A VkShaderModule object.
	*/
	VkShaderModule createShaderModule(const std::vector<char>& bytecode);
};
