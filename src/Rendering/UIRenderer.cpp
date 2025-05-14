/* UIRenderer.cpp - UI Renderer implementation.
*/

#include "UIRenderer.hpp"


UIRenderer::UIRenderer(VulkanContext& context):
    m_vkContext(context) {

    m_garbageCollector = ServiceLocator::getService<GarbageCollector>(__FUNCTION__);
    m_graphicsPipeline = ServiceLocator::getService<GraphicsPipeline>(__FUNCTION__);

	Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


UIRenderer::~UIRenderer() {}


void UIRenderer::initializeImGui(UIRenderer::Appearance appearance) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(m_vkContext.window, true);


    // Setup Dear ImGui style
    switch (appearance) {
    case IMGUI_APPEARANCE_DARK_MODE:
        ImGui::StyleColorsDark();
        break;

    case IMGUI_APPEARANCE_LIGHT_MODE:
        ImGui::StyleColorsLight();
        break;

    default:
        throw Log::RuntimeException(__FUNCTION__, __LINE__, "Unable to initialize ImGui: Invalid appearance value!");
    }


    // When viewports are enabled, we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }


    // Initialization info
    ImGui_ImplVulkan_InitInfo vkInitInfo = {};
    vkInitInfo.Instance = m_vkContext.vulkanInstance;         // Instance
    vkInitInfo.PhysicalDevice = m_vkContext.Device.physicalDevice;   // Physical device
    vkInitInfo.Device = m_vkContext.Device.logicalDevice;            // Logical device

    // Queue
    QueueFamilyIndices familyIndices = m_vkContext.Device.queueFamilies;
    vkInitInfo.QueueFamily = familyIndices.graphicsFamily.index.value();
    vkInitInfo.Queue = familyIndices.graphicsFamily.deviceQueue;

    // Pipeline cache
    vkInitInfo.PipelineCache = VK_NULL_HANDLE;

    // Descriptor pool
    std::vector<VkDescriptorPoolSize> imgui_PoolSizes = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE }
    };
    VkDescriptorPoolCreateFlags imgui_DescPoolCreateFlags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    m_graphicsPipeline->createDescriptorPool(imgui_PoolSizes, m_descriptorPool, imgui_DescPoolCreateFlags);
    vkInitInfo.DescriptorPool = m_descriptorPool;


    // Render pass & subpass
    vkInitInfo.RenderPass = m_vkContext.GraphicsPipeline.renderPass;
    vkInitInfo.Subpass = 1;

    // Image count
    vkInitInfo.MinImageCount = m_vkContext.SwapChain.minImageCount; // For some reason, ImGui does not actually use this property
    vkInitInfo.ImageCount = m_vkContext.SwapChain.minImageCount;

    // Other
    vkInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    vkInitInfo.Allocator = nullptr;

    vkInitInfo.CheckVkResultFn = [](VkResult result) {
        if (result != VK_SUCCESS) {
            throw Log::RuntimeException(__FUNCTION__, __LINE__, "An error occurred while setting up or running Dear Imgui!");
        }
    };

    ImGui_ImplVulkan_Init(&vkInitInfo);


    // Loads m_pFont
    m_pFont = io.Fonts->AddFontFromMemoryTTF((void*)DefaultFontData, static_cast<int>(DefaultFontLength), 20.0f);

    // Default to Imgui's default m_pFont if loading from memory fails
    if (m_pFont == nullptr) {
        m_pFont = io.Fonts->AddFontDefault();
    }

    ImGui_ImplVulkan_CreateFontsTexture();


    // (your code submit a queue)
    //ImGui_ImplVulkan_DestroyFontUploadObjects();

    // Implements custom style
    m_currentAppearance = appearance;
    updateAppearance(m_currentAppearance);


    CleanupTask task{};
    task.caller = __FUNCTION__;
    task.objectNames = { "ImGui destruction calls" };
    task.cleanupFunc = []() {
        ImGui_ImplVulkan_DestroyFontsTexture();
        ImGui_ImplVulkan_Shutdown();
    };

    m_garbageCollector->createCleanupTask(task);
}


void UIRenderer::refreshImGui() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_vkContext.window, &width, &height);

    QueueFamilyIndices familyIndices = m_vkContext.Device.queueFamilies;
    uint32_t queueFamily = familyIndices.graphicsFamily.index.value();

    ImGui_ImplVulkan_SetMinImageCount(m_vkContext.SwapChain.minImageCount);
    //ImGui_ImplVulkanH_CreateOrResizeWindow(m_vkContext.vulkanInstance, m_vkContext.Device.physicalDevice, m_vkContext.Device.logicalDevice, WINDOW, queueFamily, nullptr, width, height, m_vkContext.minImageCount);
}


void UIRenderer::renderFrames() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::PushFont(m_pFont);
    

    ImGui::ShowDemoWindow();
    ImGui::Begin("Astrocelerate Control Panel"); // Window title
    ImGui::Text("Welcome to Astrocelerate!"); // Basic text
    static float someValue = 0.0f;
    ImGui::SliderFloat("Simulation Speed", &someValue, 0.0f, 10.0f); // Slider
    if (ImGui::Button("Launch Simulation")) {
        // Call your simulation start function here
    }
    ImGui::End();


    ImGui::PopFont();

    ImGui::Render();

    ImGui::UpdatePlatformWindows();
    ImGui::EndFrame();
}


void UIRenderer::updateAppearance(UIRenderer::Appearance appearance) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    switch (appearance) {
    case IMGUI_APPEARANCE_DARK_MODE:
        Log::print(Log::T_VERBOSE, __FUNCTION__, "Updating GUI appearance to dark mode...");

        // Backgrounds
        colors[ImGuiCol_WindowBg] = linearRGBA(0.10f, 0.10f, 0.10f, 1.0f); // Dark gray
        colors[ImGuiCol_ChildBg] = linearRGBA(0.12f, 0.12f, 0.12f, 1.0f); // Slightly lighter gray
        colors[ImGuiCol_PopupBg] = linearRGBA(0.08f, 0.08f, 0.08f, 0.94f); // Almost black

        // Headers (collapsing sections, menus)
        colors[ImGuiCol_Header] = linearRGBA(0.25f, 0.25f, 0.25f, 1.0f);
        colors[ImGuiCol_HeaderHovered] = linearRGBA(0.30f, 0.30f, 0.30f, 1.0f);
        colors[ImGuiCol_HeaderActive] = linearRGBA(0.35f, 0.35f, 0.35f, 1.0f);

        // Borders and separators
        colors[ImGuiCol_Border] = linearRGBA(0.25f, 0.25f, 0.25f, 0.50f);
        colors[ImGuiCol_BorderShadow] = linearRGBA(0, 0, 0, 0); // Remove shadow

        // Text
        colors[ImGuiCol_Text] = linearRGBA(0.95f, 0.96f, 0.98f, 1.00f); // White
        colors[ImGuiCol_TextDisabled] = linearRGBA(0.50f, 0.50f, 0.50f, 1.00f); // Dim gray

        // Frames (inputs, sliders, etc.)
        colors[ImGuiCol_FrameBg] = linearRGBA(0.15f, 0.15f, 0.15f, 1.0f);
        colors[ImGuiCol_FrameBgHovered] = linearRGBA(0.20f, 0.20f, 0.20f, 1.0f);
        colors[ImGuiCol_FrameBgActive] = linearRGBA(0.25f, 0.25f, 0.25f, 1.0f);

        // Buttons
        colors[ImGuiCol_Button] = linearRGBA(0.20f, 0.22f, 0.27f, 1.0f); // Dark blue-gray
        colors[ImGuiCol_ButtonHovered] = linearRGBA(0.30f, 0.33f, 0.38f, 1.0f);
        colors[ImGuiCol_ButtonActive] = linearRGBA(0.35f, 0.40f, 0.45f, 1.0f);

        // Scrollbars
        colors[ImGuiCol_ScrollbarBg] = linearRGBA(0.10f, 0.10f, 0.10f, 1.0f);
        colors[ImGuiCol_ScrollbarGrab] = linearRGBA(0.20f, 0.25f, 0.30f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabHovered] = linearRGBA(0.25f, 0.30f, 0.35f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabActive] = linearRGBA(0.30f, 0.35f, 0.40f, 1.0f);

        // Tabs (used for docking)
        colors[ImGuiCol_Tab] = linearRGBA(0.18f, 0.18f, 0.18f, 1.0f);
        colors[ImGuiCol_TabHovered] = linearRGBA(0.26f, 0.26f, 0.26f, 1.0f);
        colors[ImGuiCol_TabActive] = linearRGBA(0.28f, 0.28f, 0.28f, 1.0f);
        colors[ImGuiCol_TabUnfocused] = linearRGBA(0.15f, 0.15f, 0.15f, 1.0f);
        colors[ImGuiCol_TabUnfocusedActive] = linearRGBA(0.20f, 0.20f, 0.20f, 1.0f);

        // Title bar
        colors[ImGuiCol_TitleBg] = linearRGBA(0.10f, 0.10f, 0.10f, 1.0f);
        colors[ImGuiCol_TitleBgActive] = linearRGBA(0.15f, 0.15f, 0.15f, 1.0f);
        colors[ImGuiCol_TitleBgCollapsed] = linearRGBA(0.10f, 0.10f, 0.10f, 0.75f);

        // Resize grips (bottom-right corner)
        colors[ImGuiCol_ResizeGrip] = linearRGBA(0.30f, 0.30f, 0.30f, 0.5f);
        colors[ImGuiCol_ResizeGripHovered] = linearRGBA(0.40f, 0.40f, 0.40f, 0.75f);
        colors[ImGuiCol_ResizeGripActive] = linearRGBA(0.45f, 0.45f, 0.45f, 1.0f);

        break;

    case IMGUI_APPEARANCE_LIGHT_MODE:
        Log::print(Log::T_VERBOSE, __FUNCTION__, "Updating GUI appearance to light mode...");

        break;

    default:
        throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot update ImGui appearance: Invalid appearance value!");
        break;
    }
}
