/* Renderer.cpp - Renderer implementation.
*/


#include "Renderer.hpp"


Renderer::Renderer():
    m_vulkInst(g_vkContext.vulkanInstance) {

    m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);

    // TODO: I've just realized that having multiple entity managers (which is very likely) will be problematic for the service locator.
    m_globalRegistry = ServiceLocator::GetService<Registry>(__FUNCTION__);
    m_garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);

    m_swapchainManager = ServiceLocator::GetService<VkSwapchainManager>(__FUNCTION__);
    m_bufferManager = ServiceLocator::GetService<VkBufferManager>(__FUNCTION__);
    m_commandManager = ServiceLocator::GetService<VkCommandManager>(__FUNCTION__);

    m_imguiRenderer = ServiceLocator::GetService<UIRenderer>(__FUNCTION__);

    Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


Renderer::~Renderer() {}


void Renderer::update(glm::dvec3& renderOrigin) {
    drawFrame(renderOrigin);
}


void Renderer::init() {
    initializeRenderables();
}


void Renderer::initializeRenderables() {
    m_guiRenderable = m_globalRegistry->createEntity();
    m_globalRegistry->addComponent(m_guiRenderable.id, m_guiRenderComponent);
}


void Renderer::drawFrame(glm::dvec3& renderOrigin) {
    m_guiRenderComponent.guiDrawData = ImGui::GetDrawData();

    m_globalRegistry->updateComponent(m_guiRenderable.id, m_guiRenderComponent);

    /* How a frame is drawn:
    * 1. Wait for the previous frame to finish rendering (i.e., waiting for its fence)
    * 2. After waiting, acquire a new image from the swap-chain for rendering
    * 3. If drawFrame does not end prematurely because the swap-chain is either outdated or suboptimal, then it means we are ready to start rendering the image. Only in that case do we reset the fence to ensure that only fences of images that are guaranteed to be processed are reset.
    * 4. Reset/Clear the current frame's command buffer
    * 5. Record the commands from the image
    * 6. Submit the filled-in command buffer to a queue for processing
    * 7. Send the processed data back to the swap-chain to render the image
    * 8. Update the current frame index so that the next drawFrame call will process the next image in the swap-chain
    */

    // VK_TRUE: Indicates that the vkWaitForFences should wait for all fences.
    // UINT64_MAX: The maximum time to wait (timeout) (in nanoseconds). UINT64_MAX means to wait indefinitely (i.e., to disable the timeout)
    VkResult waitResult = vkWaitForFences(g_vkContext.Device.logicalDevice, 1, &g_vkContext.SyncObjects.inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    LOG_ASSERT(waitResult == VK_SUCCESS, "Failed to wait for in-flight fence!");


    // Acquires an image from the swap-chain
    uint32_t imageIndex;
    VkResult imgAcquisitionResult = vkAcquireNextImageKHR(g_vkContext.Device.logicalDevice, g_vkContext.SwapChain.swapChain, UINT64_MAX, g_vkContext.SyncObjects.imageReadySemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (imgAcquisitionResult != VK_SUCCESS) {
        if (imgAcquisitionResult == VK_ERROR_OUT_OF_DATE_KHR || imgAcquisitionResult == VK_SUBOPTIMAL_KHR) {
            m_swapchainManager->recreateSwapchain(m_currentFrame);
            m_imguiRenderer->refreshImGui();
            return;
        }

        else {
            throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to acquire an image from the swap-chain!\nThe current image index in this frame is: " + std::to_string(imageIndex));
        }
    }


    // Only reset the fence when we're submitting work

    // After waiting, reset fence to unsignaled
    VkResult resetFenceResult = vkResetFences(g_vkContext.Device.logicalDevice, 1, &g_vkContext.SyncObjects.inFlightFences[m_currentFrame]);
    LOG_ASSERT(resetFenceResult == VK_SUCCESS, "Failed to reset fence!");


    // Records the command buffer
        // Resets the command buffer first to ensure it is able to be recorded
    VkResult cmdBufResetResult = vkResetCommandBuffer(g_vkContext.CommandObjects.graphicsCmdBuffers[m_currentFrame], 0);
    LOG_ASSERT(cmdBufResetResult == VK_SUCCESS, "Failed to reset command buffer!");


        // Updates the uniform buffers
    m_eventDispatcher->publish(Event::UpdateUBOs{
        .currentFrame = m_currentFrame,
        .renderOrigin = renderOrigin
    }, true);

        // Updates all ImGui textures (aka descriptor sets) after the current frame has been processed (i.e., its fence has been reset)
    m_imguiRenderer->updateTextures(m_currentFrame);
    
        // Records commands
    m_commandManager->recordRenderingCommandBuffer(g_vkContext.CommandObjects.graphicsCmdBuffers[m_currentFrame], imageIndex, m_currentFrame);


        // Submits the buffer to the queue
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            // Specifies the command buffer to be submitted
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &g_vkContext.CommandObjects.graphicsCmdBuffers[m_currentFrame];

            /* NOTE:
            * Each stage in waitStages[] corresponds to a semaphore in waitSemaphores[].
            */
    VkSemaphore waitSemaphores[] = {
        g_vkContext.SyncObjects.imageReadySemaphores[m_currentFrame] // Wait for the image to be available (see waitStages[0])
    };
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT // Wait for the colors to first be written to the image, because (theoretically) our vertex shader could be executed prematurely (before the image is available).
    };

            // Specifies which semaphores to wait for before execution begins
    submitInfo.waitSemaphoreCount = (sizeof(waitSemaphores) / sizeof(VkSemaphore));
    submitInfo.pWaitSemaphores = waitSemaphores;

            // Specifies which stage of the (graphics) pipeline to wait for
    submitInfo.pWaitDstStageMask = waitStages;

            // Specifies which semaphores to signal once the command buffer's execution is finished
    VkSemaphore signalSemaphores[] = {
        g_vkContext.SyncObjects.renderFinishedSemaphores[m_currentFrame]
    };
    submitInfo.signalSemaphoreCount = (sizeof(signalSemaphores) / sizeof(VkSemaphore));
    submitInfo.pSignalSemaphores = signalSemaphores;

    VkQueue graphicsQueue = g_vkContext.Device.queueFamilies.graphicsFamily.deviceQueue;
    VkResult submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, g_vkContext.SyncObjects.inFlightFences[m_currentFrame]);
    LOG_ASSERT(submitResult == VK_SUCCESS, "Failed to submit draw command buffer!");


    // To finally draw the frame, we submit the result back to the swap-chain to have it eventually show up on screen
        // Configures presentation
    VkPresentInfoKHR presentationInfo{};
    presentationInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        // Since we want to wait for the command buffer to finish execution, we take the semaphores which will be signalled and wait for them (i.e., we use signalSemaphores).
    presentationInfo.waitSemaphoreCount = (sizeof(signalSemaphores) / sizeof(VkSemaphore));
    presentationInfo.pWaitSemaphores = signalSemaphores;

        // Specifies the swap-chains to present images to, and the image index for each swap-chain (this will almost always be a single one)
    VkSwapchainKHR swapChains[] = {
        g_vkContext.SwapChain.swapChain
    };
    presentationInfo.swapchainCount = (sizeof(swapChains) / sizeof(VkSwapchainKHR));
    presentationInfo.pSwapchains = swapChains;
    presentationInfo.pImageIndices = &imageIndex;

        // Specifies an array of VkResult values to check if presentation was successful for every single swap-chain. We leave pResults as nullptr for now, since we currently have just 1 swap-chain (whose result is the return value of the vkQueuePresentKHR function)
    presentationInfo.pResults = nullptr;

    VkResult presentResult = vkQueuePresentKHR(graphicsQueue, &presentationInfo);
    if (presentResult != VK_SUCCESS) {
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
            m_swapchainManager->recreateSwapchain(m_currentFrame);
            m_imguiRenderer->refreshImGui();
            return;
        }
        else {
            throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to present swap-chain image!");
        }
    }

    // Updates current frame index
    m_currentFrame = (m_currentFrame + 1) % SimulationConsts::MAX_FRAMES_IN_FLIGHT;

}
