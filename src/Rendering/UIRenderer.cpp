/* UIRenderer.cpp - UI Renderer implementation.
*/

#include "UIRenderer.hpp"


UIRenderer::UIRenderer() {

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
    ImGui_ImplGlfw_InitForVulkan(g_vkContext.window, true);


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
    vkInitInfo.Instance = g_vkContext.vulkanInstance;         // Instance
    vkInitInfo.PhysicalDevice = g_vkContext.Device.physicalDevice;   // Physical device
    vkInitInfo.Device = g_vkContext.Device.logicalDevice;            // Logical device

    // Queue
    QueueFamilyIndices familyIndices = g_vkContext.Device.queueFamilies;
    vkInitInfo.QueueFamily = familyIndices.graphicsFamily.index.value();
    vkInitInfo.Queue = familyIndices.graphicsFamily.deviceQueue;

    // Pipeline cache
    vkInitInfo.PipelineCache = VK_NULL_HANDLE;

    // Descriptor pool
    std::vector<VkDescriptorPoolSize> imgui_PoolSizes = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE }
    };
    VkDescriptorPoolCreateFlags imgui_DescPoolCreateFlags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    VkDescriptorUtils::CreateDescriptorPool( m_descriptorPool, imgui_PoolSizes, imgui_DescPoolCreateFlags);
    vkInitInfo.DescriptorPool = m_descriptorPool;


    // Render pass & subpass
    vkInitInfo.RenderPass = g_vkContext.PresentPipeline.renderPass;
    vkInitInfo.Subpass = 0;

    // Image count
    vkInitInfo.MinImageCount = g_vkContext.SwapChain.minImageCount; // For some reason, ImGui does not actually use this property
    vkInitInfo.ImageCount = g_vkContext.SwapChain.minImageCount;

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


    // Loads default fonts
    initFonts();

    ImGui_ImplVulkan_CreateFontsTexture();

    // Implements custom style
    m_currentAppearance = appearance;
    updateAppearance(m_currentAppearance);

    auto iniBuffer = FilePathUtils::readFile(ConfigConsts::IMGUI_DEFAULT_CONFIG);
    ImGui::LoadIniSettingsFromMemory(iniBuffer.data(), iniBuffer.size());

    m_eventDispatcher->publish(Event::GUIContextIsValid{});
}


void UIRenderer::initFonts() {
    ImGuiIO& io = ImGui::GetIO();

    const float fontSize = 25.0f;
    const float iconSize = 18.0f;


    // Primary/Default text font: Roboto Regular
    g_fontContext.Roboto.regular = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.REGULAR.c_str(), fontSize, nullptr, io.Fonts->GetGlyphRangesDefault());
    LOG_ASSERT(g_fontContext.Roboto.regular, "Failed to load (default) Roboto Regular font!");

        // NOTE: Use this config for subsequent fonts, as they need to merge into the default font
    ImFontConfig mergeConfig;
    mergeConfig.MergeMode = true;   // NOTE: This merges the new font/icons into the default font
    mergeConfig.PixelSnapH = true;  // NOTE: This helps with crisp rendering of icons


    // FontAwesome icons
    const ImWchar faGlyphRanges[] = {
        ICON_MIN_FA, ICON_MAX_FA, 0
    };

    //io.Fonts->AddFontFromFileTTF(FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Fonts", "FontAwesome", "FontAwesome-6-Brands-Regular-400.otf").c_str(), iconSize, &mergeConfig, faGlyphRanges);
    //io.Fonts->AddFontFromFileTTF(FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Fonts", "FontAwesome", "FontAwesome-6-Free-Regular-400.otf").c_str(), iconSize, &mergeConfig, faGlyphRanges);
    io.Fonts->AddFontFromFileTTF(FilePathUtils::joinPaths(APP_SOURCE_DIR, "assets/Fonts", "FontAwesome", "FontAwesome-6-Free-Solid-900.otf").c_str(), iconSize, &mergeConfig, faGlyphRanges);
    

    // Roboto
    g_fontContext.Roboto.black = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.BLACK.c_str(), fontSize, &mergeConfig, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.blackItalic = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.BLACK_ITALIC.c_str(), fontSize, &mergeConfig, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.bold = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.BOLD.c_str(), fontSize, &mergeConfig, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.boldItalic = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.BOLD_ITALIC.c_str(), fontSize, &mergeConfig, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.italic = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.ITALIC.c_str(), fontSize, &mergeConfig, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.light = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.LIGHT.c_str(), fontSize, &mergeConfig, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.lightItalic = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.LIGHT_ITALIC.c_str(), fontSize, &mergeConfig, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.medium = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.MEDIUM.c_str(), fontSize, &mergeConfig, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.mediumItalic = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.MEDIUM_ITALIC.c_str(), fontSize, &mergeConfig, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.thin = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.THIN.c_str(), fontSize, &mergeConfig, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.thinItalic = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.THIN_ITALIC.c_str(), fontSize, &mergeConfig, io.Fonts->GetGlyphRangesDefault());


    io.Fonts->Build();
}


void UIRenderer::initDockspace() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);


    ImGuiWindowFlags dockspaceFlags =
        ImGuiWindowFlags_NoDocking  | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;


    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (ImGui::Begin("MainDockspace", nullptr, dockspaceFlags)) {
        ImGui::PopStyleVar(2);

        ImGui::PushFont(g_fontContext.Roboto.regular);
        m_uiPanelManager->renderMenuBar();
        ImGui::PopFont();

        ImGuiID dockspaceID = ImGui::GetID("Dockspace");
        ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);


        ImGui::End();
    }
}


void UIRenderer::refreshImGui() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(g_vkContext.window, &width, &height);

    QueueFamilyIndices familyIndices = g_vkContext.Device.queueFamilies;
    uint32_t queueFamily = familyIndices.graphicsFamily.index.value();

    ImGui_ImplVulkan_SetMinImageCount(g_vkContext.SwapChain.minImageCount);
    //ImGui_ImplVulkanH_CreateOrResizeWindow(g_vkContext.vulkanInstance, g_vkContext.Device.physicalDevice, g_vkContext.Device.logicalDevice, WINDOW, queueFamily, nullptr, width, height, g_vkContext.minImageCount);
}


void UIRenderer::renderFrames(uint32_t currentFrame) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    {
        initDockspace();

        ImGui::PushFont(g_fontContext.Roboto.regular);
        m_uiPanelManager->updatePanels(currentFrame);
        ImGui::PopFont();

        // Multi-viewport support
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
    ImGui::EndFrame();

    ImGui::Render();
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
