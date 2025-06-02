/* PipelineBuilder.hpp - Defines a framework with which pipelines are built.
*/

#pragma once

#include <glfw_vulkan.hpp>

#include <vector>

#include <Core/ServiceLocator.hpp>
#include <Core/GarbageCollector.hpp>


class PipelineBuilder {
public:
	PipelineBuilder() {
		m_garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);
	};
	~PipelineBuilder() = default;

	VkPipelineDynamicStateCreateInfo* dynamicStateCreateInfo;
    VkPipelineInputAssemblyStateCreateInfo* inputAssemblyCreateInfo;
	VkPipelineViewportStateCreateInfo* viewportStateCreateInfo;
    VkPipelineRasterizationStateCreateInfo* rasterizerCreateInfo;
    VkPipelineMultisampleStateCreateInfo* multisampleStateCreateInfo;
	VkPipelineDepthStencilStateCreateInfo* depthStencilStateCreateInfo;
	VkPipelineColorBlendStateCreateInfo* colorBlendStateCreateInfo;
	VkPipelineTessellationStateCreateInfo* tessellationStateCreateInfo;
	VkPipelineVertexInputStateCreateInfo* vertexInputStateCreateInfo;

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkRenderPass renderPass;
	

	inline VkPipeline buildGraphicsPipeline(VkDevice& logicalDevice) {
		VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO; // Specify the pipeline as the graphics pipeline

		// Shader stage
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCreateInfo.pStages = shaderStages.data();

		// Fixed-function states
		pipelineCreateInfo.pDynamicState = dynamicStateCreateInfo;
		pipelineCreateInfo.pInputAssemblyState = inputAssemblyCreateInfo;
		pipelineCreateInfo.pViewportState = viewportStateCreateInfo;
		pipelineCreateInfo.pRasterizationState = rasterizerCreateInfo;
		pipelineCreateInfo.pMultisampleState = multisampleStateCreateInfo;
		pipelineCreateInfo.pDepthStencilState = depthStencilStateCreateInfo;
		pipelineCreateInfo.pColorBlendState = colorBlendStateCreateInfo;
		pipelineCreateInfo.pTessellationState = tessellationStateCreateInfo;
		pipelineCreateInfo.pVertexInputState = vertexInputStateCreateInfo;

		// Render pass
		pipelineCreateInfo.renderPass = renderPass;
		pipelineCreateInfo.subpass = 0; // Index of the subpass
		/* NOTE:
		* It is also possible to use other render passes with this pipeline instead of this specific instance, but they have to be compatible with m_renderPass. The requirements for compatibility are described here: https://docs.vulkan.org/spec/latest/chapters/renderpass.html#renderpass-compatibility
		*/

		// Pipeline properties
			/* NOTE:
			* Vulkan allows you to create a new graphics pipeline by deriving from an existing pipeline.
			* The idea of pipeline derivatives is that it is less expensive to set up pipelines when they have much functionality in common with an existing pipeline and switching between pipelines from the same parent can also be done quicker.
			*
			* You can either specify the handle of an existing pipeline with basePipelineHandle or reference another pipeline that is about to be created by index with basePipelineIndex.
			* These values are only used if the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is also specified in the flags field of VkGraphicsPipelineCreateInfo.
			*/
			// Right now, there is only a single pipeline, so we'll specify the handle and index as null and -1 (an invalid index)
		//pipelineCreateInfo.flags;
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCreateInfo.basePipelineIndex = -1;

		pipelineCreateInfo.layout = pipelineLayout;

		VkPipeline graphicsPipeline;
		VkResult result = vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline);

		CleanupTask task{};
		task.caller = __FUNCTION__;
		task.objectNames = { VARIABLE_NAME(graphicsPipeline) };
		task.vkObjects = { logicalDevice, graphicsPipeline };
		task.cleanupFunc = [logicalDevice, graphicsPipeline]() { vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr); };

		m_garbageCollector->createCleanupTask(task);


		if (result != VK_SUCCESS) {
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to create graphics pipeline!");
		}

		return graphicsPipeline;
	}

	
private:
	std::shared_ptr<GarbageCollector> m_garbageCollector;
};