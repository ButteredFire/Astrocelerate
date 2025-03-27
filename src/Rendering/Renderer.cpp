/* Renderer.cpp - Renderer implementation.
*/


#include "Renderer.hpp"


Renderer::Renderer(VulkanContext &context, VkSwapchainManager& swapchainMgrInstance, RenderPipeline& renderPipelineInstance):
    vulkInst(context.vulkanInstance),
    swapchainMgr(swapchainMgrInstance),
    renderPipeline(renderPipelineInstance),
    vkContext(context) {

    Log::print(Log::T_INFO, __FUNCTION__, "Initializing...");

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(vkContext.window, true);


    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();


    // When viewports are enabled, we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    

    // Initialization info
    ImGui_ImplVulkan_InitInfo vkInitInfo = {};
    vkInitInfo.Instance = vkContext.vulkanInstance;
    vkInitInfo.PhysicalDevice = vkContext.physicalDevice;
    vkInitInfo.Device = vkContext.logicalDevice;
    
	QueueFamilyIndices familyIndices = VkDeviceManager::getQueueFamilies(vkContext.physicalDevice, vkContext.vkSurface);
    vkInitInfo.QueueFamily = familyIndices.graphicsFamily.index.value();
    vkInitInfo.Queue = familyIndices.graphicsFamily.deviceQueue;

    //vkInitInfo.PipelineCache = YOUR_PIPELINE_CACHE;
    //vkInitInfo.DescriptorPool = YOUR_DESCRIPTOR_POOL;
    vkInitInfo.Subpass = 1;
    vkInitInfo.MinImageCount = vkContext.minImageCount;
    vkInitInfo.ImageCount = 2;
    vkInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    vkInitInfo.Allocator = nullptr;

    //vkInitInfo.CheckVkResultFn = check_vk_result;
    //ImGui_ImplVulkan_Init(&vkInitInfo, wd->RenderPass);
    // (this gets a bit more complicated, see example app for full reference)
    //ImGui_ImplVulkan_CreateFontsTexture(YOUR_COMMAND_BUFFER);
    // (your code submit a queue)
    //ImGui_ImplVulkan_DestroyFontUploadObjects();
}


Renderer::~Renderer() {
    
};

void Renderer::update() {
    drawFrame();
}


void Renderer::drawFrame() {
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
    VkResult waitResult = vkWaitForFences(vkContext.logicalDevice, 1, &vkContext.RenderPipeline.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    if (waitResult != VK_SUCCESS) {
        throw Log::RuntimeException(__FUNCTION__, "Failed to wait for in-flight fence!");
    }

    // Acquires an image from the swap-chain
    uint32_t imageIndex;
    VkResult imgAcquisitionResult = vkAcquireNextImageKHR(vkContext.logicalDevice, vkContext.swapChain, UINT64_MAX, vkContext.RenderPipeline.imageReadySemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (imgAcquisitionResult != VK_SUCCESS) {
        if (imgAcquisitionResult == VK_ERROR_OUT_OF_DATE_KHR || imgAcquisitionResult == VK_SUBOPTIMAL_KHR) {
            swapchainMgr.recreateSwapchain(renderPipeline);
            return;
        }

        else {
            throw Log::RuntimeException(__FUNCTION__, "Failed to acquire an image from the swap-chain!\nThe current image index in this frame is: " + std::to_string(imageIndex));
        }
    }

    // Only reset the fence when we're submitting work

    // After waiting, reset fence to unsignaled
    VkResult resetFenceResult = vkResetFences(vkContext.logicalDevice, 1, &vkContext.RenderPipeline.inFlightFences[currentFrame]);
    if (resetFenceResult != VK_SUCCESS) {
        throw Log::RuntimeException(__FUNCTION__, "Failed to reset fence!");
    }


    // Records the command buffer
        // Resets the command buffer first to ensure it is able to be recorded
    VkResult cmdBufResetResult = vkResetCommandBuffer(vkContext.RenderPipeline.graphicsCmdBuffers[currentFrame], 0);
    if (cmdBufResetResult != VK_SUCCESS) {
        throw Log::RuntimeException(__FUNCTION__, "Failed to reset command buffer!");
    }

        // Records commands
    renderPipeline.recordCommandBuffer(vkContext.RenderPipeline.graphicsCmdBuffers[currentFrame], imageIndex);

        // Submits the buffer to the queue
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            // Specifies the command buffer to be submitted
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkContext.RenderPipeline.graphicsCmdBuffers[currentFrame];

            /* NOTE:
            * Each stage in waitStages[] corresponds to a semaphore in waitSemaphores[].
            */
    VkSemaphore waitSemaphores[] = {
        vkContext.RenderPipeline.imageReadySemaphores[currentFrame] // Wait for the image to be available (see waitStages[0])
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
        vkContext.RenderPipeline.renderFinishedSemaphores[currentFrame]
    };
    submitInfo.signalSemaphoreCount = (sizeof(signalSemaphores) / sizeof(VkSemaphore));
    submitInfo.pSignalSemaphores = signalSemaphores;

    VkQueue graphicsQueue = vkContext.queueFamilies.graphicsFamily.deviceQueue;
    VkResult submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, vkContext.RenderPipeline.inFlightFences[currentFrame]);
    if (submitResult != VK_SUCCESS) {
        throw Log::RuntimeException(__FUNCTION__, "Failed to submit draw command buffer!");
    }

    // To finally draw the frame, we submit the result back to the swap-chain to have it eventually show up on screen
        // Configures presentation
    VkPresentInfoKHR presentationInfo{};
    presentationInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        // Since we want to wait for the command buffer to finish execution, we take the semaphores which will be signalled and wait for them (i.e., we use signalSemaphores).
    presentationInfo.waitSemaphoreCount = (sizeof(signalSemaphores) / sizeof(VkSemaphore));
    presentationInfo.pWaitSemaphores = signalSemaphores;

        // Specifies the swap-chains to present images to, and the image index for each swap-chain (this will almost always be a single one)
    VkSwapchainKHR swapChains[] = {
        vkContext.swapChain
    };
    presentationInfo.swapchainCount = (sizeof(swapChains) / sizeof(VkSwapchainKHR));
    presentationInfo.pSwapchains = swapChains;
    presentationInfo.pImageIndices = &imageIndex;

        // Specifies an array of VkResult values to check if presentation was successful for every single swap-chain. We leave pResults as nullptr for now, since we currently have just 1 swap-chain (whose result is the return value of the vkQueuePresentKHR function)
    presentationInfo.pResults = nullptr;

    VkResult presentResult = vkQueuePresentKHR(graphicsQueue, &presentationInfo);
    if (presentResult != VK_SUCCESS) {
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
            swapchainMgr.recreateSwapchain(renderPipeline);
            return;
        }
        else {
            throw Log::RuntimeException(__FUNCTION__, "Failed to present swap-chain image!");
        }
    }

    // Updates current frame index
    currentFrame = (currentFrame + 1) % SimulationConsts::MAX_FRAMES_IN_FLIGHT;
}
