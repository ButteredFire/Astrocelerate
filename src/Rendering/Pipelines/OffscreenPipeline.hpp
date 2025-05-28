/* OffscreenPipeline.hpp - Manages a graphics pipeline for offscreen rendering.
*/

#pragma once

#include <glfw_vulkan.hpp>

#include <vector>

#include <Core/Constants.h>
#include <Core/GarbageCollector.hpp>
#include <Core/ServiceLocator.hpp>

#include <Rendering/Textures/TextureManager.hpp>
#include <Rendering/Pipelines/PipelineBuilder.hpp>

#include <Vulkan/VkBufferManager.hpp>

#include <Utils/SystemUtils.hpp>


class OffscreenPipeline {
public:
	OffscreenPipeline(VulkanContext& context);
	~OffscreenPipeline() = default;

	void init();

	// Descriptor handling could be abstracted further
	void createDescriptorPool(std::vector<VkDescriptorPoolSize> poolSizes, VkDescriptorPool& descriptorPool, VkDescriptorPoolCreateFlags createFlags = VkDescriptorPoolCreateFlags());


	/* Does the (depth) format contain a stencil component? */
	inline static bool formatHasStencilComponent(VkFormat format) {
		return (format == VK_FORMAT_D32_SFLOAT_S8_UINT) ||
			(format == VK_FORMAT_D24_UNORM_S8_UINT);
	}

private:
	VulkanContext& m_vkContext;

	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<GarbageCollector> m_garbageCollector;
	std::shared_ptr<VkBufferManager> m_bufferManager;

	VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;

	// Shaders
		// Vertex shader
	std::vector<char> m_vertShaderBytecode;
	VkShaderModule m_vertShaderModule = VK_NULL_HANDLE;

	VkPipelineVertexInputStateCreateInfo m_vertInputState{};
	VkVertexInputBindingDescription m_vertBindingDescription = VkVertexInputBindingDescription();
	std::vector<VkVertexInputAttributeDescription> m_vertAttribDescriptions{};

	// Fragment shader
	std::vector<char> m_fragShaderBytecode;
	VkShaderModule m_fragShaderModule = VK_NULL_HANDLE;
	std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;

	// Render pass
	VkRenderPass m_renderPass = VK_NULL_HANDLE;

	// Dynamic states
	std::vector<VkDynamicState> m_dynamicStates;
	VkPipelineDynamicStateCreateInfo m_dynamicStateCreateInfo{};

	// Assembly state
	VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyCreateInfo{};

	// Multisampling state
	VkPipelineMultisampleStateCreateInfo m_multisampleStateCreateInfo{};

	// Depth stencil state
	VkPipelineDepthStencilStateCreateInfo m_depthStencilStateCreateInfo{};

	// Depth buffering
	VkImage m_depthImage = VK_NULL_HANDLE;
	VmaAllocation m_depthImageAllocation = VK_NULL_HANDLE;
	VkImageView m_depthImageView = VK_NULL_HANDLE;

	// Tessellation state
	VkPipelineTessellationStateCreateInfo m_tessStateCreateInfo{};

	// Descriptors
	VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> m_descriptorSets;

	uint32_t m_descriptorCount = 0;

	// Pipeline layout
	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;


	void bindEvents();

	void createGraphicsPipeline();
	void createPipelineLayout();

	void createRenderPass();


	/* Sets up descriptors. This method is an aggregate of multiple methods pertaining to descriptors. */
	void setUpDescriptors();


	/* Creates a descriptor set layout.
		@param layoutBindings: A vector of descriptor set layout binings.
	*/
	void createDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding> layoutBindings);


	void createDescriptorSets();


	/* Creates the shader stage of the graphics pipeline from compiled SPIR-V shader files. */
	void initShaderStage();


	/* Initializes dynamic states.
		While most of the pipeline state needs to be baked into the pipeline state, a limited amount of the state can actually be changed without recreating the pipeline at draw time. Examples are the size of the viewport, line width and blend constants.
		Binding dynamic states via a Vk...CreateInfo structure causes the configuration of these values to be ignored and we will be able (and required) to specify the data at drawing time. This results in a more flexible setup.
	*/
	void initDynamicStates();
	
	
	/* Initializes input assembly state.
		The input assembly state specifies:
		1. What kind of geometry will be drawn from the vertices (CreateInfo::topology)
		2. Whether primitive restart should be enabled
	*/
	void initInputAssemblyState();


	/* Initializes multisampling state.
		TODO: FINISH FUNCTION
	*/
	void initMultisamplingState();


	/* Initializes depth-stencil testing.
		Depth-stencil testing is used to determine how fragments (i.e., "potential pixels") are rendered based on their depth and stencil values.
		This process ensures that objects closer to the camera are rendered correctly, and hides fragments that are (logically) obfuscated in 3D space (e.g., overlapping fragments -> choose to only render fragments closer to the camera -> create depth).

		Specifically:
			- Depth testing:
				+ Purpose: Ensures that only the closest fragments to the camera are rendered.
				+ Under the hood: Each fragment has a depth value (i.e., the z-coordinate) that is compared against the depth buffer (a per-pixel storage of depth values). Based on the comparison (e.g., less-than, greater-than), the fragment is either kept or discarded.
				+ Common use: Creates depth by preventing objects behind other objects (from the camera perspective) from being drawn over them.

			- Stencil testing:
				+ Purpose: Controls whether a fragment should be drawn based on stencil buffer values.
				+ Under the hood: The stencil buffer stores integer values for each pixel. A stencil test compares those values against a reference value using a specified operation (e.g., equal, not-equal). If the fragment "passes" the test, it gets rendered. Otherwise, it is discarded.
				+ Common use: Enables effects such as masking, outlining, rendering specific regions of the screen, etc..
	*/
	void initDepthStencilState();


	/* Initializes tessellation state.
		TODO: FINISH FUNCTION
		Tessellation is disabled for now. To enable it, specify the input assembly state's topology as PATCH_LIST, change the framebuffer attachment sample count in createRenderPass(), and add the tessellation create info struct to createGraphicsPipeline()
	*/
	void initTessellationState();


	/* Creates depth buffering resources. */
	void initDepthBufferingResources();


	/* Creates a shader module to pass the code to the pipeline.
		@param bytecode: The SPIR-V shader file bytecode data.

		@return A VkShaderModule object.
	*/
	VkShaderModule createShaderModule(const std::vector<char>& bytecode);
};