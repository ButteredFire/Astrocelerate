#pragma once

#include "IVisualizer.hpp"

#include <array>
#include <unordered_map>

#include <Platform/Vulkan/Contexts.hpp>
#include <Platform/External/GLFWVulkan.hpp>


/* Rendering module for orbit trajectories. */
class OrbitVisualizer : public IVisualizer {
public:
	OrbitVisualizer(const Ctx::VkRenderDevice *renderDeviceCtx, const Ctx::VkWindow *windowCtx, const Ctx::OffscreenPipeline *offscreenData, const Buffer::BufferAlloc *orbitVertBuffer, std::unordered_map<uint32_t, std::pair<size_t, uint32_t>> orbitVertexOffsets);
	~OrbitVisualizer() override = default;

	void init(std::array<VkCommandBuffer, SimulationConst::MAX_FRAMES_IN_FLIGHT> secondCmdBufs) override;
	void prepareFrame(uint32_t frameIdx, const Buffer::FramePacket &framePacket) override;
	void render(uint32_t frameIdx) override;

private:
	const Ctx::VkRenderDevice *m_renderDeviceCtx;
	const Ctx::VkWindow *m_windowCtx;

	const Ctx::OffscreenPipeline *m_offscreenData;

	const Buffer::BufferAlloc *m_orbitVertBuffer;
	std::unordered_map<uint32_t, std::pair<size_t, uint32_t>> m_orbitVertexOffsets;
	
	VkDeviceSize m_alignedGlobalUBOSize;

	std::array<VkCommandBuffer, SimulationConst::MAX_FRAMES_IN_FLIGHT> m_secondCmdBufs;
};