#pragma once

#include "IVisualizer.hpp"

#include <Core/Application/Resources/CleanupManager.hpp>
#include <Core/Application/Resources/ServiceLocator.hpp>

#include <Engine/Registry/ECS/ECS.hpp>
#include <Engine/Registry/ECS/Components/CoreComponents.hpp>
#include <Engine/Registry/ECS/Components/RenderComponents.hpp>
#include <Engine/Registry/ECS/Components/PhysicsComponents.hpp>
#include <Engine/Registry/ECS/Components/TelemetryComponents.hpp>
#include <Engine/Rendering/Data/Geometry.hpp>

#include <Platform/Vulkan/Contexts.hpp>
#include <Platform/Vulkan/VkBufferManager.hpp>


/* Rendering module for general scene geometry.
*/
class GeometryVisualizer : public IVisualizer {
public:
	GeometryVisualizer(const Ctx::VkRenderDevice *renderDeviceCtx, const Ctx::VkWindow *windowCtx, const Ctx::OffscreenPipeline *offscreenData, const Geometry::GeometryData *geomData, const Buffer::BufferAlloc *globalVertBuffer, const Buffer::BufferAlloc *globalIdxBuffer, std::shared_ptr<VkBufferManager> bufMgr);
	~GeometryVisualizer() override;

	void init(std::array<VkCommandBuffer, SimulationConst::MAX_FRAMES_IN_FLIGHT> secondCmdBufs) override;
	void prepareFrame(uint32_t frameIdx, const Buffer::FramePacket &framePacket) override;
	void render(uint32_t frameIdx) override;

private:
	std::shared_ptr<CleanupManager> m_cleanupManager;
	std::shared_ptr<ECSRegistry> m_ecsRegistry;

	std::shared_ptr<VkBufferManager> m_bufManager;

	const Ctx::VkRenderDevice *m_renderDeviceCtx;
	const Ctx::VkWindow *m_windowCtx;

	const Geometry::GeometryData *m_geomData;

	// Pipeline data
	VkPipeline m_offscreenPipeline;
	VkPipelineLayout m_offscreenPipelineLayout;
	VkRenderPass m_offscreenRenderPass;
	std::vector<VkFramebuffer> m_offscreenFrameBuffers;


	// Buffers & descriptor sets
	const Buffer::BufferAlloc *m_globalVertBufAlloc;
	const Buffer::BufferAlloc *m_globalIdxBufAlloc;

	struct FrameMemResource {
		Buffer::BufferAlloc bufAlloc;
		VkDescriptorSet descriptorSet;
		std::vector<Math::Interval<uint32_t>> meshRanges;
	};
	std::array<FrameMemResource, SimulationConst::MAX_FRAMES_IN_FLIGHT> m_objectUBOs;

	Buffer::BufferAlloc m_materialUBOAlloc;
	std::vector<VkDescriptorSet> m_perFrameDescriptorSets;
	VkDescriptorSet m_materialDescriptorSet;
	VkDescriptorSet m_texArrayDescriptorSet;

	VkDeviceSize m_minUBOAlignment;
	VkDeviceSize m_alignedObjectUBOSize;
	VkDeviceSize m_alignedMaterialSize;

	std::array<VkCommandBuffer, SimulationConst::MAX_FRAMES_IN_FLIGHT> m_secondCmdBufs;

	ResourceID m_visualizerID;


	void initResources();
};