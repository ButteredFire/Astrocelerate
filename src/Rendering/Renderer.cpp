/* Renderer.cpp - Renderer implementation.
*/


#include "Renderer.hpp"


Renderer::Renderer(VkCoreResourcesManager *coreResources, VkSwapchainManager *swapchainMgr, VkCommandManager *commandMgr, VkSyncManager *syncMgr, UIRenderer *uiRenderer):
    m_coreResources(coreResources),
    m_swapchainManager(swapchainMgr),
    m_commandManager(commandMgr),
    m_syncManager(syncMgr),
    m_uiRenderer(uiRenderer) {

    m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
    m_globalRegistry = ServiceLocator::GetService<Registry>(__FUNCTION__);
    m_garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);

    bindEvents();

    init();

    Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


Renderer::~Renderer() {}


void Renderer::bindEvents() {
    static EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<Renderer>();

    m_eventDispatcher->subscribe<UpdateEvent::SessionStatus>(selfIndex,
        [this](const UpdateEvent::SessionStatus &event) {
            using enum UpdateEvent::SessionStatus::Status;

            switch (event.sessionStatus) {
            case PREPARE_FOR_RESET:
                m_sessionReady = false;
                break;

            case INITIALIZED:
                m_sessionReady = true;
                break;
            }
        }
    );


    m_eventDispatcher->subscribe<RecreationEvent::Swapchain>(selfIndex, 
        [this](const RecreationEvent::Swapchain &event) {
            m_swapchainDeferredCleanupIDs = event.deferredDestructionList;
        }
    );
}


void Renderer::init() {
    m_imageReadySemaphores = m_syncManager->getImageReadySemaphores();
    m_inFlightFences = m_syncManager->getInFlightFences();

    m_renderFinishedSemaphores = m_syncManager->getRenderFinishedSemaphores();
    m_renderFinishedSemaphoreCleanupIDs = m_syncManager->getRenderFinishedSemaphoreCleanupIDs();

    m_graphicsCommandBuffers = m_commandManager->getGraphicsCommandBuffers();
}


void Renderer::update(glm::dvec3& renderOrigin) {
    drawFrame(renderOrigin);
}


void Renderer::preRenderUpdate(uint32_t currentFrame, glm::dvec3 &renderOrigin) {
    if (!m_sessionReady)
        return;

    // Updates the uniform buffers
    m_eventDispatcher->dispatch(UpdateEvent::PerFrameBuffers{
        .currentFrame = m_currentFrame,
        .renderOrigin = renderOrigin
    }, true);

    // GUI updates
    m_uiRenderer->preRenderUpdate(m_currentFrame);
}


void Renderer::drawFrame(glm::dvec3& renderOrigin) {
    static VkQueue lastQueue = VK_NULL_HANDLE;

    /* How a frame is drawn:
        1. Wait for the previous frame to finish rendering (i.e., waiting for its fence)
        2. After waiting, acquire a new image from the swap-chain for rendering
        3. If drawFrame does not end prematurely because the swap-chain is either outdated or suboptimal, then it means we are ready to start rendering the image. Only in that case do we reset the fence to ensure that only fences of images that are guaranteed to be processed are reset.
        4. Reset/Clear the current frame's command buffer
        5. Record the commands from the image
        6. Submit the filled-in command buffer to a queue for processing
        7. Send the processed data back to the swap-chain to render the image
        8. Update the current frame index so that the next drawFrame call will process the next image in the swap-chain
    */


    // VK_TRUE: Indicates that the vkWaitForFences should wait for all fences.
    // UINT64_MAX: The maximum time to wait (timeout) (in nanoseconds). UINT64_MAX means to wait indefinitely (i.e., to disable the timeout)
    VkResult waitResult = vkWaitForFences(m_coreResources->getLogicalDevice(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    LOG_ASSERT(waitResult == VK_SUCCESS, "Failed to wait for in-flight fence!");


    
    if (!m_swapchainDeferredCleanupIDs.empty()) {
        vkQueueWaitIdle(lastQueue);

        // Destroy old swapchain resources
        for (const auto &taskID : m_swapchainDeferredCleanupIDs)
            m_garbageCollector->executeCleanupTask(taskID);
        m_swapchainDeferredCleanupIDs.clear();

        // Destroy old per-image signal semaphores
        for (const auto &taskID : m_renderFinishedSemaphoreCleanupIDs)
            m_garbageCollector->executeCleanupTask(taskID);
        m_renderFinishedSemaphoreCleanupIDs.clear();

        // Create new such semaphores
        m_syncManager->createPerImageSemaphores();
        m_renderFinishedSemaphores = m_syncManager->getRenderFinishedSemaphores();
        m_renderFinishedSemaphoreCleanupIDs = m_syncManager->getRenderFinishedSemaphoreCleanupIDs();
    }



    // After waiting, reset in-flight fence to unsignaled
    VkResult resetFenceResult = vkResetFences(m_coreResources->getLogicalDevice(), 1, &m_inFlightFences[m_currentFrame]);
    LOG_ASSERT(resetFenceResult == VK_SUCCESS, "Failed to reset fence!");



    // Acquires an image from the swap-chain
    uint32_t imageIndex;
    VkResult imgAcquisitionResult = vkAcquireNextImageKHR(m_coreResources->getLogicalDevice(), m_swapchainManager->getSwapChain(), UINT64_MAX, m_imageReadySemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (imgAcquisitionResult != VK_SUCCESS) {
        if (imgAcquisitionResult == VK_ERROR_OUT_OF_DATE_KHR || imgAcquisitionResult == VK_SUBOPTIMAL_KHR) {
            m_swapchainManager->recreateSwapchain(m_currentFrame, m_inFlightFences);
            m_uiRenderer->refreshImGui();
            return;
        }

        else {
            throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to acquire an image from the swap-chain!\nThe current image index in this frame is: " + std::to_string(imageIndex));
        }
    }



    // Records the command buffer
        // Resets the command buffer first to ensure it is able to be recorded
    VkResult cmdBufResetResult = vkResetCommandBuffer(m_graphicsCommandBuffers[m_currentFrame], 0);
    LOG_ASSERT(cmdBufResetResult == VK_SUCCESS, "Failed to reset command buffer!");

        // Perform any updates prior to command buffer recording
    preRenderUpdate(m_currentFrame, renderOrigin);
    
        // Records commands
    m_commandManager->recordRenderingCommandBuffer(m_graphicsCommandBuffers[m_currentFrame], imageIndex, m_currentFrame);


    // Submits the buffer to the queue
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            // Specifies the command buffer to be submitted
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_graphicsCommandBuffers[m_currentFrame];

            // NOTE: Each stage in waitStages[] corresponds to a semaphore in waitSemaphores[].
    VkSemaphore waitSemaphores[] = {
        m_imageReadySemaphores[m_currentFrame] // Wait for the image to be available (see waitStages[0])
    };
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT // Wait for the colors to first be written to the image, because (theoretically) our vertex shader could be executed prematurely (before the image is available).
    };

            // Specifies which semaphores to wait for before execution begins
    submitInfo.waitSemaphoreCount = SIZE_OF(waitSemaphores);
    submitInfo.pWaitSemaphores = waitSemaphores;

            // Specifies which stage of the (graphics) pipeline to wait for
    submitInfo.pWaitDstStageMask = waitStages;

            // Specifies which semaphores to signal once the command buffer's execution is finished
    VkSemaphore signalSemaphores[] = {
        m_renderFinishedSemaphores[imageIndex]
    };
    submitInfo.signalSemaphoreCount = SIZE_OF(signalSemaphores);
    submitInfo.pSignalSemaphores = signalSemaphores;


    VkQueue graphicsQueue = m_coreResources->getQueueFamilyIndices().graphicsFamily.deviceQueue;
    lastQueue = graphicsQueue;

    VkResult submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]);
    LOG_ASSERT(submitResult == VK_SUCCESS, "Failed to submit draw command buffer!");



    // To finally draw the frame, we submit the result back to the swap-chain to have it eventually show up on screen
        // Configures presentation
    VkPresentInfoKHR presentationInfo{};
    presentationInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        // Since we want to wait for the command buffer to finish execution, we take the semaphores which will be signaled and wait for them (i.e., we use signalSemaphores).
    presentationInfo.waitSemaphoreCount = SIZE_OF(signalSemaphores);
    presentationInfo.pWaitSemaphores = signalSemaphores;

        // Specifies the swap-chains to present images to, and the image index for each swap-chain (this will almost always be a single one)
    VkSwapchainKHR swapChains[] = {
        m_swapchainManager->getSwapChain()
    };
    presentationInfo.swapchainCount = SIZE_OF(swapChains);
    presentationInfo.pSwapchains = swapChains;
    presentationInfo.pImageIndices = &imageIndex;

        // Specifies an array of VkResult values to check if presentation was successful for every single swap-chain. We leave pResults as nullptr for now, since we currently have just 1 swap-chain (whose result is the return value of the vkQueuePresentKHR function)
    presentationInfo.pResults = nullptr;


    VkResult presentResult = vkQueuePresentKHR(graphicsQueue, &presentationInfo);
    if (presentResult != VK_SUCCESS) {
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
            m_swapchainManager->recreateSwapchain(imageIndex, m_inFlightFences);
            m_uiRenderer->refreshImGui();
            return;
        }
        else {
            throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to present swap-chain image!");
        }
    }


    // Updates current frame index
    m_currentFrame = (m_currentFrame + 1) % SimulationConsts::MAX_FRAMES_IN_FLIGHT;
}
