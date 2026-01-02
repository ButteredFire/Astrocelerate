/* OffscreenPipeline.hpp - Manages a graphics pipeline for offscreen rendering.
*/

#pragma once

#include <External/GLFWVulkan.hpp>

#include <vector>

#include <Core/Application/ResourceManager.hpp>
#include <Core/Data/Constants.h>
#include <Core/Engine/ServiceLocator.hpp>

#include <Rendering/Textures/TextureManager.hpp>
#include <Rendering/Pipelines/PipelineBuilder.hpp>

#include <Vulkan/VkImageManager.hpp>

#include <Utils/Vulkan/VkFormatUtils.hpp>
#include <Utils/Vulkan/VkDescriptorUtils.hpp>



class OffscreenPipeline {
public:
	OffscreenPipeline(VkCoreResourcesManager *coreResources, VkSwapchainManager *swapchainMgr);
	~OffscreenPipeline() = default;

	void init();


	inline VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }
	inline VkPipeline getPipeline() const { return m_graphicsPipeline; }
	
	inline VkRenderPass getRenderPass() const { return m_renderPass; }

private:
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<ResourceManager> m_resourceManager;

	VkSwapchainManager *m_swapchainManager;

	VkPhysicalDevice m_physicalDevice;
	VkPhysicalDeviceProperties m_deviceProperties;
	VkDevice m_logicalDevice;

	VkExtent2D m_swapchainExtent;

	
	// Pipeline
	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
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

	// Viewport state & scissor rectangle
	VkViewport m_viewport{};
	VkPipelineViewportStateCreateInfo m_viewportStateCreateInfo{};

	VkRect2D m_scissorRectangle{};

	// Rasterization state
	VkPipelineRasterizationStateCreateInfo m_rasterizerCreateInfo{};

	// Multisampling state
	VkPipelineMultisampleStateCreateInfo m_multisampleStateCreateInfo{};

	// Depth stencil state
	VkPipelineDepthStencilStateCreateInfo m_depthStencilStateCreateInfo{};

	// Color blending state
	VkPipelineColorBlendAttachmentState m_colorBlendAttachment{};
	VkPipelineColorBlendStateCreateInfo m_colorBlendCreateInfo{};

	// Depth buffering
	VkImage m_depthImage = VK_NULL_HANDLE;
	VmaAllocation m_depthImgAllocation = VK_NULL_HANDLE;
	VkImageView m_depthImgView = VK_NULL_HANDLE;

	// Tessellation state
	VkPipelineTessellationStateCreateInfo m_tessStateCreateInfo{};

	// Descriptors
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
	std::vector<VkDescriptorSet> m_perFrameDescriptorSets;

	VkDescriptorSet m_pbrDescriptorSet;
	VkDescriptorSet m_texArrayDescriptorSet;

	// Offscreen resources
	VmaAllocation m_colorImgAlloc{};

	const size_t m_OFFSCREEN_RESOURCE_COUNT = SimulationConst::MAX_FRAMES_IN_FLIGHT;
	std::vector<VkImage> m_colorImages;
	std::vector<VkImageView> m_colorImgViews;
	std::vector<VkSampler> m_colorImgSamplers;
	std::vector<VkFramebuffer> m_colorImgFramebuffers;

	std::vector<uint32_t> m_offscreenCleanupIDs;


	// Session data
	std::vector<CleanupID> m_sessionCleanupIDs;	// Used to execute old cleanup tasks on new session initialization
	bool m_sessionReady = false;


	void bindEvents();

	void createGraphicsPipeline();
	void createPipelineLayout();

	void createRenderPass();


	/* Sets up descriptors. This method is an aggregate of multiple methods pertaining to descriptors. */
	void setUpDescriptors();


	/* Creates a descriptor set layout.
		@param bindingCount: The number of bindings.
		@param layoutBindings: A pointer to the binding (or an array of bindings).
		@param pNext: Vulkan struct chain for the descriptor set creation info.

		@return The newly created layout.
	*/
	VkDescriptorSetLayout createDescriptorSetLayout(uint32_t bindingCount, VkDescriptorSetLayoutBinding *layoutBindings, VkDescriptorSetLayoutCreateFlags layoutFlags, const void *pNext);


	/* Creates descriptor sets.
		@param descriptorSetCount: The number of descriptor sets to create.
		@param descriptorSets: A pointer to the descriptor sets (or an array of descriptor sets).
		@param descriptorSetLayouts: A pointer to the descriptor set layouts (or an array of descriptor set layouts).
		@param descriptorPool: The descriptor pool to allocate the sets from.
		@param pNext: Vulkan struct chain for the descriptor set allocation info.
	*/
	void createDescriptorSets(uint32_t descriptorSetCount, VkDescriptorSet *descriptorSets, VkDescriptorSetLayout *descriptorSetLayouts, VkDescriptorPool descriptorPool, const void *pNext);


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


	/* Initializes viewport state and scissor rectangles.
		A viewport essentially defines a region of the framebuffer that the output will be rendered to (i.e., the transformation from the image to the buffer).
		A scissor rectangle defines the region in which pixels are actually stored (pixels outside of which will be ignored by the rasterizer).
	*/
	void initViewportState();


	/* Initializes the rasterizer.
		The rasterizer turns the geometry shaped by vertices (that are created from the vertex shader) into fragments to be colored in the fragment shader.
		It also performs depth testing, face culling and the scissor test.
		It can be configured to output fragments that fill entire polygons or just the edges (i.e., wireframe rendering).

		NOTE ON WIREFRAME RENDERING:
		- Switching between polygon fill mode (normal rendering) and polygon line mode (wireframe rendering) requires creating an entirely new pipeline, since the rasterization state cannot be made dynamic.
		- An alternative is to use mesh shaders. In modern Vulkan (e.g., Vulkan 1.3+ with mesh shading), we could theoretically implement a custom mesh shader that dynamically renders as wireframe. However, this is an advanced topic and requires shader-based geometry processing.
	*/
	void initRasterizationState();


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


	/* Initializes color blending.
		After a fragment shader has returned a color, it needs to be combined with the color that is already in the framebuffer. This transformation is known as color blending.
	*/
	void initColorBlendingState();


	/* Initializes tessellation state.
		TODO: FINISH FUNCTION
		Tessellation is disabled for now. To enable it, specify the input assembly state's topology as PATCH_LIST, change the framebuffer attachment sample count in createRenderPass(), and add the tessellation create info struct to createGraphicsPipeline()
	*/
	void initTessellationState();

	void initDepthBufferingResources();
	
	void recreateOffscreenResources(uint32_t width, uint32_t height);
	void initOffscreenColorResources(uint32_t width, uint32_t height);
	void initOffscreenSampler();
	void initOffscreenFramebuffer(uint32_t width, uint32_t height);


	/* Creates a shader module to pass the code to the pipeline.
		@param bytecode: The SPIR-V shader file bytecode data.

		@return A VkShaderModule object.
	*/
	VkShaderModule createShaderModule(const std::vector<char>& bytecode);
};