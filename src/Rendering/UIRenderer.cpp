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


void UIRenderer::initImGui() {
    ImGuiTheme::Appearance defaultAppearance = ImGuiTheme::Appearance::IMGUI_APPEARANCE_DARK_MODE;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(g_vkContext.window, true);


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
        // Sampler to draw the GUI
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
        
        // Samplers to draw offscreen resources (for rendering onto the viewport)
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(g_vkContext.OffscreenResources.images.size()) }
    };
    VkDescriptorPoolCreateFlags imgui_DescPoolCreateFlags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    VkDescriptorUtils::CreateDescriptorPool(m_descriptorPool, imgui_PoolSizes, imgui_DescPoolCreateFlags);
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
    if (!IN_DEBUG_MODE) {
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
        // Refer to ImGui::StyleColorsDark() and ImGui::StyleColorsLight() for more information
    ImGuiTheme::ApplyTheme(defaultAppearance);
    g_appContext.GUI.currentAppearance = defaultAppearance;


    auto iniBuffer = FilePathUtils::readFile(ConfigConsts::IMGUI_DEFAULT_CONFIG);
    ImGui::LoadIniSettingsFromMemory(iniBuffer.data(), iniBuffer.size());

    m_eventDispatcher->publish(Event::GUIContextIsValid{});
}


void UIRenderer::initFonts() {
    ImGuiIO& io = ImGui::GetIO();

    const float fontSize = 25.0f;
    const float iconSize = 18.0f;

    // Glyph ranges
    static const ImWchar commonRanges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement (GetGlyphRangesDefault)
        0x0100, 0x017F, // Latin Extended-A
        0x0180, 0x024F, // Latin Extended-B
        0x0300, 0x036F, // Combining Diacritical Marks

        // Vietnamese
        0x0020, 0x00FF, // Basic Latin
        0x0102, 0x0103,
        0x0110, 0x0111,
        0x0128, 0x0129,
        0x0168, 0x0169,
        0x01A0, 0x01A1,
        0x01AF, 0x01B0,
        0x1EA0, 0x1EF9,

        0, // NULL terminator
    };


    // Primary/Default text font: Roboto Regular
    // NOTE: It is the default font because it is the first font to be loaded.
    g_fontContext.Roboto.regular = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.REGULAR.c_str(), fontSize, nullptr, commonRanges);
    LOG_ASSERT(g_fontContext.Roboto.regular, "Failed to load (default) Roboto Regular font!");

        // IMPORTANT: Since only the icons use this merge config (meaning that they get merged into the default font),
        // the icons will only be available when the default font is used.
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
    g_fontContext.Roboto.black = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.BLACK.c_str(), fontSize, nullptr, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.blackItalic = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.BLACK_ITALIC.c_str(), fontSize, nullptr, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.bold = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.BOLD.c_str(), fontSize, nullptr, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.boldItalic = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.BOLD_ITALIC.c_str(), fontSize, nullptr, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.italic = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.ITALIC.c_str(), fontSize, nullptr, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.light = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.LIGHT.c_str(), fontSize, nullptr, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.lightItalic = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.LIGHT_ITALIC.c_str(), fontSize, nullptr, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.medium = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.MEDIUM.c_str(), fontSize, nullptr, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.mediumItalic = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.MEDIUM_ITALIC.c_str(), fontSize, nullptr, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.thin = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.THIN.c_str(), fontSize, nullptr, io.Fonts->GetGlyphRangesDefault());
    g_fontContext.Roboto.thinItalic = io.Fonts->AddFontFromFileTTF(FontConsts::Roboto.THIN_ITALIC.c_str(), fontSize, nullptr, io.Fonts->GetGlyphRangesDefault());


    io.Fonts->Build();
}


void UIRenderer::initDockspace() {
    ImGuiViewport* windowViewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(windowViewport->Pos);
    ImGui::SetNextWindowSize(windowViewport->Size);
    ImGui::SetNextWindowViewport(windowViewport->ID);


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


void UIRenderer::updateTextures(uint32_t currentFrame) {
    m_uiPanelManager->updateViewportTexture(currentFrame);
}
