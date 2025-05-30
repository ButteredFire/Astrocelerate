/* PresentPipeline.hpp - Manages a barebones graphics pipeline solely for presentation of swapchain images to the screen.
*/

#pragma once

#include <glfw_vulkan.hpp>

#include <Core/ServiceLocator.hpp>
#include <Core/LoggingManager.hpp>
#include <Core/EventDispatcher.hpp>
#include <Core/GarbageCollector.hpp>

#include <CoreStructs/Contexts.hpp>

#include <Rendering/Pipelines/PipelineBuilder.hpp>

#include <Vulkan/VkBufferManager.hpp>

#include <Utils/VulkanUtils.hpp>


class PresentPipeline {
public:
	PresentPipeline(VulkanContext& context);
	~PresentPipeline() = default;

	void init();

private:
	VulkanContext& m_vkContext;

	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<GarbageCollector> m_garbageCollector;
	std::shared_ptr<VkBufferManager> m_bufferManager;

	VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;

	// Render pass
	VkRenderPass m_renderPass = VK_NULL_HANDLE;

	// Viewport state & scissor rectangle
	VkViewport m_viewport{};
	VkPipelineViewportStateCreateInfo m_viewportStateCreateInfo{};

	VkRect2D m_scissorRectangle{};

	// Rasterization state
	VkPipelineRasterizationStateCreateInfo m_rasterizerCreateInfo{};

	// Color blending state
	VkPipelineColorBlendAttachmentState m_colorBlendAttachment{};
	VkPipelineColorBlendStateCreateInfo m_colorBlendCreateInfo{};

	// Pipeline layout
	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;



	/* Creates the graphics pipeline. */
	void createGraphicsPipeline();


	/* Initializes the pipeline layout. */
	void createPipelineLayout();


	/* Creates a render pass.
		A render pass is a collection of rendering operations that all share/use the same framebuffer of the image to be rendered.
		It defines how the rendering commands are organized and executed.
	*/
	void createRenderPass();


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


	/* Initializes color blending.
		After a fragment shader has returned a color, it needs to be combined with the color that is already in the framebuffer. This transformation is known as color blending.
	*/
	void initColorBlendingState();
};