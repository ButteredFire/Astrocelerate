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
			+ "\n" + "The file may not be in the directory " + enquote(workingDirectory) + '.'
			+ "\n" + "To change the working directory, please specify the full path to the file."
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
	void cleanup();

private:
	VulkanContext& vkContext;

	VkPipeline graphicsPipeline = VK_NULL_HANDLE;

	// Shaders
		// Vertex shader
	std::vector<char> vertShaderBytecode;
	VkShaderModule vertShaderModule = VK_NULL_HANDLE;
	VkPipelineVertexInputStateCreateInfo vertInputState{};

		// Fragment shader
	std::vector<char> fragShaderBytecode;
	VkShaderModule fragShaderModule = VK_NULL_HANDLE;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	// Render pass
	VkRenderPass renderPass = VK_NULL_HANDLE;

	// Dynamic states
	std::vector<VkDynamicState> dynamicStates;
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};

	// Assembly state
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};

	// Viewport state & scissor rectangle
	VkViewport viewport{};
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};

	VkRect2D scissorRectangle{};

	// Rasterization state
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo{};

	// Multisampling state
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};

	// Depth stencil state
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};

	// Color blending state
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo{};

	// Tessellation state
	VkPipelineTessellationStateCreateInfo tessStateCreateInfo{};

	// Pipeline layout
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;



	/* Creates the graphics pipeline. */
	void createGraphicsPipeline();


	/* Initializes the pipeline layout. */
	void createPipelineLayout();


	/* Creates a render pass. */
	void createRenderPass();


	/* Creates the shader stage of the graphics pipeline from compiled SPIR-V shader files. */
	void initShaderStage();


	/* Initializes dynamic states.
	* While most of the pipeline state needs to be baked into the pipeline state, a limited amount of the state can actually be changed without recreating the pipeline at draw time. Examples are the size of the viewport, line width and blend constants.
	* Binding dynamic states via a Vk...CreateInfo structure causes the configuration of these values to be ignored and we will be able (and required) to specify the data at drawing time. This results in a more flexible setup.
	*/
	void initDynamicStates();


	/* Initializes input assembly state.
	* The input assembly state specifies:
	* 1. What kind of geometry will be drawn from the vertices (CreateInfo::topology)
	* 2. Whether primitive restart should be enabled
	*/
	void initInputAssemblyState();


	/* Initializes viewport state and scissor rectangles. 
	* A viewport essentially defines a region of the framebuffer that the output will be rendered to (i.e., the transformation from the image to the buffer).
	* A scissor rectangle defines the region in which pixels are actually stored (pixels outside of which will be ignored by the rasterizer).
	*/
	void initViewportState();


	/* Initializes the rasterizer. 
	* The rasterizer turns the geometry shaped by vertices (that are created from the vertex shader) into fragments to be colored in the fragment shader.
	* It also performs depth testing, face culling and the scissor test.
	* It can be configured to output fragments that fill entire polygons or just the edges (i.e., wireframe rendering). 
	* 
	* NOTE ON WIREFRAME RENDERING:
	* - Switching between polygon fill mode (normal rendering) and polygon line mode (wireframe rendering) requires creating an entirely new pipeline, since the rasterization state cannot be made dynamic.
	* - An alternative is to use mesh shaders. In modern Vulkan (e.g., Vulkan 1.3+ with mesh shading), we could theoretically implement a custom mesh shader that dynamically renders as wireframe. However, this is an advanced topic and requires shader-based geometry processing.
	*/
	void initRasterizationState();


	/* Initializes multisampling state. 
	* TODO: FINISH FUNCTION
	*/
	void initMultisamplingState();


	/* Initializes depth stencil testing. 
	* TODO: FINISH FUNCTION
	* Depth stencil testing is disabled for now. To enable it, change the framebuffer attachment stencilLoadOp and stencilStoreOp in createRenderPass(), and add the depth stencil state create info struct to createGraphicsPipeline()
	*/
	void initDepthStencilState();


	/* Initializes color blending. 
	* After a fragment shader has returned a color, it needs to be combined with the color that is already in the framebuffer. This transformation is known as color blending.
	*/
	void initColorBlendingState();


	/* Initializes tessellation state. 
	* TODO: FINISH FUNCTION
	* Tessellation is disabled for now. To enable it, specify the input assembly state's topology as PATCH_LIST, change the framebuffer attachment sample count in createRenderPass(), and add the tessellation create info struct to createGraphicsPipeline()
	*/
	void initTessellationState();
	
	
	/* Creates a shader module to pass the code to the pipeline.
	* @param bytecode: The SPIR-V shader file bytecode data.
	* @return A VkShaderModule object.
	*/
	VkShaderModule createShaderModule(const std::vector<char>& bytecode);
};
