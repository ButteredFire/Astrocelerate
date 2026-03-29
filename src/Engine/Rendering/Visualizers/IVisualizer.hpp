#pragma once

#include <array>

#include <Core/Data/Constants.h>

#include <Engine/Rendering/Data/Buffer.hpp>		// Buffer::FramePacket

#include <Platform/Vulkan/Data/Buffer.hpp>		// Buffer::BufferAlloc


/* Base class implementation of a Visualizer - a mostly self-contained rendering module with the necessary data to render a particular part of the global scene.
*/
class IVisualizer {
public:
	virtual ~IVisualizer() = default;

	virtual void init(std::array<VkCommandBuffer, SimulationConst::MAX_FRAMES_IN_FLIGHT> secondCmdBufs) = 0;
	virtual void prepareFrame(uint32_t frameIdx, const Buffer::FramePacket &framePacket) = 0;
	virtual void render(uint32_t frameIdx) = 0;
};