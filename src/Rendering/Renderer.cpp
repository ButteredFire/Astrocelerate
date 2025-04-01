/* Renderer.cpp - Renderer implementation.
*/


#include "Renderer.hpp"


Renderer::Renderer(VulkanContext& context, VkSwapchainManager& swapchainMgr, BufferManager& bufMgr, GraphicsPipeline& graphicsPipelineInstance, RenderPipeline& renderPipelineInstance):
    vulkInst(context.vulkanInstance),
    swapchainManager(swapchainMgr),
    bufferManager(bufMgr),
    graphicsPipeline(graphicsPipelineInstance),
    renderPipeline(renderPipelineInstance),
    vkContext(context) {

    Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


Renderer::~Renderer() {
    ImGui_ImplVulkan_DestroyFontsTexture();
    ImGui_ImplVulkan_Shutdown();
};

void Renderer::update() {
    drawFrame();
}


void Renderer::init() {
    configureDearImGui();
}


void Renderer::configureDearImGui() {
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
    vkInitInfo.Instance = vkContext.vulkanInstance;         // Instance
    vkInitInfo.PhysicalDevice = vkContext.physicalDevice;   // Physical device
    vkInitInfo.Device = vkContext.logicalDevice;            // Logical device

        // Queue
    QueueFamilyIndices familyIndices = vkContext.queueFamilies;
    vkInitInfo.QueueFamily = familyIndices.graphicsFamily.index.value();
    vkInitInfo.Queue = familyIndices.graphicsFamily.deviceQueue;

        // Pipeline cache
    vkInitInfo.PipelineCache = VK_NULL_HANDLE;

        // Descriptor pool
    std::vector<VkDescriptorPoolSize> imgui_PoolSizes = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE }
    };
    VkDescriptorPoolCreateFlags imgui_DescPoolCreateFlags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    graphicsPipeline.createDescriptorPool(imgui_PoolSizes, imgui_DescriptorPool, imgui_DescPoolCreateFlags);
    vkInitInfo.DescriptorPool = imgui_DescriptorPool;


        // Render pass & subpass
    vkInitInfo.RenderPass = vkContext.GraphicsPipeline.renderPass;
    vkInitInfo.Subpass = 1;

        // Image count
    vkInitInfo.MinImageCount = vkContext.minImageCount; // For some reason, ImGui does not actually use this property
    vkInitInfo.ImageCount = vkContext.minImageCount;
    
        // Other
    vkInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    vkInitInfo.Allocator = nullptr;

    vkInitInfo.CheckVkResultFn = [](VkResult result) {
        if (result != VK_SUCCESS) {
            throw Log::RuntimeException(__FUNCTION__, "An error occurred while setting up or running Dear Imgui!");
        }
        };

    ImGui_ImplVulkan_Init(&vkInitInfo);


    // Loads font
        // (this gets a bit more complicated, see example app for full reference)
    //ImGui_ImplVulkan_CreateFontsTexture(YOUR_COMMAND_BUFFER);
    font = io.Fonts->AddFontFromMemoryTTF((void*)DefaultFontData, static_cast<int>(DefaultFontLength), 20.0f);

        // Default to Imgui's default font if loading from memory fails
    if (font == nullptr) {
        font = io.Fonts->AddFontDefault();
    }
    /*
    const char* fontPath = "C:\\Users\\HP\\Documents\\Visual Studio 2022\\Fonts\\Fragment_Mono\\FragmentMono-Regular.ttf";
    font = io.Fonts->AddFontFromFileTTF(fontPath, 20.0f);
    if (!std::filesystem::exists(fontPath))
        io.Fonts->AddFontDefault();
    */

    ImGui_ImplVulkan_CreateFontsTexture();


    // (your code submit a queue)
    //ImGui_ImplVulkan_DestroyFontUploadObjects();

    // Implements custom style
    ImVec4* colors = style.Colors;
    // Backgrounds
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f); // Dark gray
    colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f); // Slightly lighter gray
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f); // Almost black

    // Headers (collapsing sections, menus)
    colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);

    // Borders and separators
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0); // Remove shadow

    // Text
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f); // White
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); // Dim gray

    // Frames (inputs, sliders, etc.)
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);

    // Buttons
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.22f, 0.27f, 1.0f); // Dark blue-gray
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.33f, 0.38f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.40f, 0.45f, 1.0f);

    // Scrollbars
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.30f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.30f, 0.35f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.30f, 0.35f, 0.40f, 1.0f);

    // Tabs (used for docking)
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.26f, 0.26f, 1.0f);
    colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.0f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);

    // Title bar
    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 0.75f);

    // Resize grips (bottom-right corner)
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.30f, 0.30f, 0.30f, 0.5f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.75f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);
}


void Renderer::refreshDearImgui(){
    int width = 0, height = 0;
    glfwGetFramebufferSize(vkContext.window, &width, &height);

    QueueFamilyIndices familyIndices = vkContext.queueFamilies;
    uint32_t queueFamily = familyIndices.graphicsFamily.index.value();

    ImGui_ImplVulkan_SetMinImageCount(vkContext.minImageCount);
    //ImGui_ImplVulkanH_CreateOrResizeWindow(vkContext.vulkanInstance, vkContext.physicalDevice, vkContext.logicalDevice, WINDOW, queueFamily, nullptr, width, height, vkContext.minImageCount);
}


void Renderer::imgui_RenderDemoWindow() {
    ImGui::ShowDemoWindow();

    ImGui::Begin("Astrocelerate Control Panel"); // Window title

    ImGui::Text("Welcome to Astrocelerate!"); // Basic text
    static float someValue = 0.0f;
    ImGui::SliderFloat("Simulation Speed", &someValue, 0.0f, 10.0f); // Slider

    if (ImGui::Button("Launch Simulation")) {
        // Call your simulation start function here
    }

    ImGui::End();
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
            swapchainManager.recreateSwapchain(renderPipeline);
            refreshDearImgui();
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


    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::PushFont(font);
    imgui_RenderDemoWindow();
    ImGui::PopFont();

    ImGui::Render();


    // Records the command buffer
        // Resets the command buffer first to ensure it is able to be recorded
    VkResult cmdBufResetResult = vkResetCommandBuffer(vkContext.RenderPipeline.graphicsCmdBuffers[currentFrame], 0);
    if (cmdBufResetResult != VK_SUCCESS) {
        throw Log::RuntimeException(__FUNCTION__, "Failed to reset command buffer!");
    }

        // Records commands
    renderPipeline.recordCommandBuffer(vkContext.RenderPipeline.graphicsCmdBuffers[currentFrame], imageIndex, currentFrame);


        // Updates the uniform buffer
    bufferManager.updateUniformBuffer(currentFrame);

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
            swapchainManager.recreateSwapchain(renderPipeline);
            refreshDearImgui();
            return;
        }
        else {
            throw Log::RuntimeException(__FUNCTION__, "Failed to present swap-chain image!");
        }
    }

    // Updates current frame index
    currentFrame = (currentFrame + 1) % SimulationConsts::MAX_FRAMES_IN_FLIGHT;
}
