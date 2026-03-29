/* VkCommandManager.hpp - Manages command pools and command buffers.
*/

#pragma once

#include <vector>
#include <memory>
#include <barrier>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <filesystem>


#include <Core/Data/Constants.h>
#include <Core/Application/IO/LoggingManager.hpp>
#include <Core/Application/Resources/CleanupManager.hpp>
#include <Core/Application/Resources/ServiceLocator.hpp>

#include <Platform/Vulkan/Contexts.hpp>
#include <Platform/Vulkan/Utils/VkCommandUtils.hpp>

#include <Engine/Systems/RenderSystem.hpp>
#include <Engine/Registry/ECS/ECS.hpp>
#include <Engine/Registry/ECS/Components/RenderComponents.hpp>
#include <Engine/Registry/Event/EventDispatcher.hpp>


class VkCommandManager {
public:
	VkCommandManager(const Ctx::VkRenderDevice *renderDeviceCtx, const Ctx::VkWindow *windowCtx, const Ctx::OffscreenPipeline *offscreenData);
	~VkCommandManager() = default;


	inline std::vector<VkCommandBuffer> getGraphicsCommandBuffers() const { return m_graphicsCmdBuffers; }
	inline std::vector<VkCommandBuffer> getTransferCommandBuffers() const { return m_transferCmdBuffers; }


	void init();


	struct RenderBufferData {
		uint32_t currentFrame;
		uint32_t imageIndex;
		VkCommandBuffer buffer;
		std::vector<VkClearValue> clearValues;
	};


	RenderBufferData beginRenderBuffer(uint32_t currentFrame, uint32_t imageIndex, VkCommandBuffer &buffer);
		void executeOffscreenPass(RenderBufferData &renderBuf, const std::vector<VkCommandBuffer> &secondaryBuffers = {});
		void executePresentPass(RenderBufferData &renderBuf, const std::vector<VkCommandBuffer> &secondaryBuffers = {});
	void endRenderBuffer(RenderBufferData &renderBuf);

private:
	std::shared_ptr<EventDispatcher> m_eventDispatcher;
	std::shared_ptr<CleanupManager> m_cleanupManager;

	const Ctx::VkRenderDevice *m_renderDeviceCtx;
	const Ctx::VkWindow *m_windowCtx;
	const Ctx::OffscreenPipeline *m_offscreenData;


	// Command pools manage the memory that is used to store the buffers
	// Command buffers are allocated from them
	VkCommandPool m_graphicsCmdPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_graphicsCmdBuffers;

	VkCommandPool m_transferCmdPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_transferCmdBuffers;
};