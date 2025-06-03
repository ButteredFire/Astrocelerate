/* UIRenderer.cpp - UI Renderer implementation.
*/

#include "UIRenderer.hpp"


UIRenderer::UIRenderer(VulkanContext& context):
    m_vkContext(context) {

    m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);
    m_garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);
    m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);

    m_uiPanelManager = ServiceLocator::GetService<UIPanelManager>(__FUNCTION__);

    initImGui();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


UIRenderer::~UIRenderer() {}


void UIRenderer::initImGui(UIRenderer::Appearance appearance) {
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
    ImGui_ImplVulkan_InitInfo vkInitInfo{};
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
    VkDescriptorPoolCreateFlags imgui_DescPoolCreateFlags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    VkDescriptorUtils::CreateDescriptorPool(m_vkContext, m_descriptorPool, imgui_PoolSizes, imgui_DescPoolCreateFlags);
    vkInitInfo.DescriptorPool = m_descriptorPool;


    // Render pass & subpass
    vkInitInfo.RenderPass = m_vkContext.PresentPipeline.renderPass;
    vkInitInfo.Subpass = 0;

    // Image count
    vkInitInfo.MinImageCount = m_vkContext.SwapChain.minImageCount; // For some reason, ImGui does not actually use this property
    vkInitInfo.ImageCount = m_vkContext.SwapChain.minImageCount;

    // Other
    vkInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    vkInitInfo.Allocator = nullptr;

        // Actually show the real error origin in debug mode instead of a flashy error message box
    if (!inDebugMode) {
        vkInitInfo.CheckVkResultFn = [](VkResult result) {
            if (result != VK_SUCCESS) {
                throw Log::RuntimeException(__FUNCTION__, __LINE__, "An error occurred while setting up or running Dear Imgui!");
            }
        };
    }

    CleanupTask task{};
    task.caller = __FUNCTION__;
    task.objectNames = { "ImGui destruction calls" };
    task.cleanupFunc = []() {
        ImGui::SaveIniSettingsToDisk(ConfigConsts::IMGUI_DEFAULT_CONFIG.c_str());
        ImGui_ImplVulkan_DestroyFontsTexture();
        ImGui_ImplVulkan_Shutdown();
    };

    m_garbageCollector->createCleanupTask(task);


    ImGui_ImplVulkan_Init(&vkInitInfo);


    // Loads m_pFont
    m_pFont = io.Fonts->AddFontFromMemoryTTF((void*)DefaultFontData, static_cast<int>(DefaultFontLength), 20.0f);

    // Default to Imgui's default m_pFont if loading from memory fails
    if (m_pFont == nullptr) {
        m_pFont = io.Fonts->AddFontDefault();
    }

    ImGui_ImplVulkan_CreateFontsTexture();

    // Implements custom style
    m_currentAppearance = appearance;
    updateAppearance(m_currentAppearance);

    auto iniBuffer = FilePathUtils::readFile(ConfigConsts::IMGUI_DEFAULT_CONFIG);
    ImGui::LoadIniSettingsFromMemory(iniBuffer.data(), iniBuffer.size());

    m_eventDispatcher->publish(Event::GUIContextIsValid{});
}


void UIRenderer::initDockspace() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);


    ImGuiWindowFlags viewportFlags =
        ImGuiWindowFlags_NoDocking  | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;


    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("MainDockspace", nullptr, viewportFlags);
    ImGui::PopStyleVar(2);

    ImGui::PushFont(m_pFont);
    m_uiPanelManager->renderMenuBar();
    ImGui::PopFont();

    ImGuiID dockspaceID = ImGui::GetID("Dockspace");
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);


    ImGui::End();
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

    initDockspace();


    ImGui::PushFont(m_pFont);
    m_uiPanelManager->updatePanels();
    ImGui::PopFont();


    ImGui::Render();

    // Multi-viewport support
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    ImGui::EndFrame();
}


void UIRenderer::updateAppearance(UIRenderer::Appearance appearance) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    switch (appearance) {
    case IMGUI_APPEARANCE_DARK_MODE:
        Log::Print(Log::T_VERBOSE, __FUNCTION__, "Updating GUI appearance to dark mode...");

        // Backgrounds
        colors[ImGuiCol_WindowBg] = ColorUtils::sRGBToLinear(0.10f, 0.10f, 0.10f, 1.0f); // Dark gray
        colors[ImGuiCol_MenuBarBg] = colors[ImGuiCol_WindowBg];
        style.Colors[ImGuiCol_DockingEmptyBg] = colors[ImGuiCol_WindowBg];
        colors[ImGuiCol_ChildBg] = ColorUtils::sRGBToLinear(0.12f, 0.12f, 0.12f, 1.0f); // Slightly lighter gray
        colors[ImGuiCol_PopupBg] = ColorUtils::sRGBToLinear(0.08f, 0.08f, 0.08f, 0.94f); // Almost black

        // Headers (collapsing sections, menus)
        colors[ImGuiCol_Header] = ColorUtils::sRGBToLinear(0.25f, 0.25f, 0.25f, 1.0f);
        colors[ImGuiCol_HeaderHovered] = ColorUtils::sRGBToLinear(0.30f, 0.30f, 0.30f, 1.0f);
        colors[ImGuiCol_HeaderActive] = ColorUtils::sRGBToLinear(0.35f, 0.35f, 0.35f, 1.0f);

        // Borders and separators
        colors[ImGuiCol_Border] = ColorUtils::sRGBToLinear(0.25f, 0.25f, 0.25f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ColorUtils::sRGBToLinear(0, 0, 0, 0); // Remove shadow

        // Text
        colors[ImGuiCol_Text] = ColorUtils::sRGBToLinear(0.95f, 0.96f, 0.98f, 1.00f); // White
        colors[ImGuiCol_TextDisabled] = ColorUtils::sRGBToLinear(0.50f, 0.50f, 0.50f, 1.00f); // Dim gray

        // Frames (inputs, sliders, etc.)
        colors[ImGuiCol_FrameBg] = ColorUtils::sRGBToLinear(0.15f, 0.15f, 0.15f, 1.0f);
        colors[ImGuiCol_FrameBgHovered] = ColorUtils::sRGBToLinear(0.20f, 0.20f, 0.20f, 1.0f);
        colors[ImGuiCol_FrameBgActive] = ColorUtils::sRGBToLinear(0.25f, 0.25f, 0.25f, 1.0f);

        // Buttons
        colors[ImGuiCol_Button] = ColorUtils::sRGBToLinear(0.20f, 0.22f, 0.27f, 1.0f); // Dark blue-gray
        colors[ImGuiCol_ButtonHovered] = ColorUtils::sRGBToLinear(0.30f, 0.33f, 0.38f, 1.0f);
        colors[ImGuiCol_ButtonActive] = ColorUtils::sRGBToLinear(0.35f, 0.40f, 0.45f, 1.0f);

        // Scrollbars
        colors[ImGuiCol_ScrollbarBg] = ColorUtils::sRGBToLinear(0.10f, 0.10f, 0.10f, 1.0f);
        colors[ImGuiCol_ScrollbarGrab] = ColorUtils::sRGBToLinear(0.20f, 0.25f, 0.30f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ColorUtils::sRGBToLinear(0.25f, 0.30f, 0.35f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabActive] = ColorUtils::sRGBToLinear(0.30f, 0.35f, 0.40f, 1.0f);

        // Tabs (used for docking)
        colors[ImGuiCol_Tab] = ColorUtils::sRGBToLinear(0.18f, 0.18f, 0.18f, 1.0f);
        colors[ImGuiCol_TabHovered] = ColorUtils::sRGBToLinear(0.26f, 0.26f, 0.26f, 1.0f);
        colors[ImGuiCol_TabActive] = ColorUtils::sRGBToLinear(0.28f, 0.28f, 0.28f, 1.0f);
        colors[ImGuiCol_TabUnfocused] = ColorUtils::sRGBToLinear(0.15f, 0.15f, 0.15f, 1.0f);
        colors[ImGuiCol_TabUnfocusedActive] = ColorUtils::sRGBToLinear(0.20f, 0.20f, 0.20f, 1.0f);

        // Title bar
        colors[ImGuiCol_TitleBg] = ColorUtils::sRGBToLinear(0.10f, 0.10f, 0.10f, 1.0f);
        colors[ImGuiCol_TitleBgActive] = ColorUtils::sRGBToLinear(0.15f, 0.15f, 0.15f, 1.0f);
        colors[ImGuiCol_TitleBgCollapsed] = ColorUtils::sRGBToLinear(0.10f, 0.10f, 0.10f, 0.75f);

        // Resize grips (bottom-right corner)
        colors[ImGuiCol_ResizeGrip] = ColorUtils::sRGBToLinear(0.30f, 0.30f, 0.30f, 0.5f);
        colors[ImGuiCol_ResizeGripHovered] = ColorUtils::sRGBToLinear(0.40f, 0.40f, 0.40f, 0.75f);
        colors[ImGuiCol_ResizeGripActive] = ColorUtils::sRGBToLinear(0.45f, 0.45f, 0.45f, 1.0f);

        break;

    case IMGUI_APPEARANCE_LIGHT_MODE:
        Log::Print(Log::T_VERBOSE, __FUNCTION__, "Updating GUI appearance to light mode...");

        break;

    default:
        throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot update ImGui appearance: Invalid appearance value!");
        break;
    }
}
