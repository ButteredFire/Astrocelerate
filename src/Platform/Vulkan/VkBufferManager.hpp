#pragma once

#include <array>
#include <chrono>
#include <string>
#include <limits>
#include <iostream>
#include <optional>


#include <Core/Utils/SpaceUtils.hpp>
#include <Core/Utils/SystemUtils.hpp>
#include <Core/Utils/FilePathUtils.hpp>
#include <Core/Application/IO/LoggingManager.hpp>

#include <Platform/Vulkan/Contexts.hpp>
#include <Platform/Vulkan/Data/Buffer.hpp>
#include <Platform/Vulkan/Utils/VkBufferUtils.hpp>

#include <Engine/Rendering/Data/Buffer.hpp>


class VkBufferManager {
public:
    VkBufferManager(const Ctx::VkRenderDevice *renderDeviceCtx);
    ~VkBufferManager();


    /* Allocates a buffer of the specified size, usage, and memory usage.
        @param bufSize: The size of the buffer to allocate (in bytes).
        @param usage: The intended usage of the buffer (e.g., vertex buffer, index buffer, uniform buffer).
        @param memUsage: The intended memory usage of the buffer (e.g., GPU-only, CPU-to-GPU).

		@return: The allocated buffer and metadata.
    */
    Buffer::BufferAlloc allocate(VkDeviceSize bufSize, VkBufferUsageFlags usage, Buffer::MemIntent memUsage);

private:
    const Ctx::VkRenderDevice *m_renderDeviceCtx;

    std::vector<void *> m_mappedBufData;
};
