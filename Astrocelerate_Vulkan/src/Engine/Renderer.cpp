/* Renderer.cpp - Renderer implementation.
*/


#include "Renderer.hpp"


Renderer::Renderer(VulkanContext &context):
    vulkInst(context.vulkanInstance), 
    vkContext(context) {

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(vkContext.window, true);
    
    ImGui_ImplVulkan_InitInfo vkInitInfo = {};
    vkInitInfo.Instance = vkContext.vulkanInstance;
    vkInitInfo.PhysicalDevice = vkContext.physicalDevice;
    vkInitInfo.Device = vkContext.logicalDevice;
    
    std::cout << "Successful GUI setup.\n";
    
    /*
    vkInitInfo.QueueFamily = YOUR_QUEUE_FAMILY;
    vkInitInfo.Queue = YOUR_QUEUE;
    vkInitInfo.PipelineCache = YOUR_PIPELINE_CACHE;
    vkInitInfo.DescriptorPool = YOUR_DESCRIPTOR_POOL;
    vkInitInfo.Subpass = 0;
    vkInitInfo.MinImageCount = 2;
    vkInitInfo.ImageCount = 2;
    vkInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    vkInitInfo.Allocator = YOUR_ALLOCATOR;
    vkInitInfo.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&vkInitInfo, wd->RenderPass);
    // (this gets a bit more complicated, see example app for full reference)
    ImGui_ImplVulkan_CreateFontsTexture(YOUR_COMMAND_BUFFER);
    // (your code submit a queue)
    ImGui_ImplVulkan_DestroyFontUploadObjects();
    */
}


Renderer::~Renderer() {
    
};

void Renderer::update() {
    
}
