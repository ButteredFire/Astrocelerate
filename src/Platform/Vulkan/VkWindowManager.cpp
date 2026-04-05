#include "VkWindowManager.hpp"


VkWindowManager::VkWindowManager(GLFWwindow *window, VkCoreResourcesManager *coreResourcesMgr) :
    m_window(window), m_coreResourcesMgr(coreResourcesMgr),
    m_swapchainMgr(window, coreResourcesMgr) {

    m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);

    bindEvents();


    m_renderDeviceCtx = m_coreResourcesMgr->getCoreContext();
    m_windowCtx = std::make_unique<Ctx::VkWindow>();

    m_swapchainMgr.init();

    updateCtx();
};


void VkWindowManager::bindEvents() {
    static EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<VkWindowManager>();

    m_eventDispatcher->subscribe<InitEvent::PresentPipeline>(selfIndex,
        [this](const InitEvent::PresentPipeline &event) {
            m_windowCtx->presentRenderPass = event.renderPass;

            m_swapchainMgr.createFrameBuffers(event.renderPass);

            updateCtx();
        }
    );
}


const Ctx::VkWindow *VkWindowManager::recreateSwapchain(GLFWwindow *oldWindow, GLFWwindow *newWindow) {
    LOG_ASSERT((oldWindow && newWindow) || (!oldWindow && !newWindow), "Cannot recreate swapchain: pointers to the old and new GLFW windows can only either be both null (for same-window swapchain recreation) or both valid (for swapchain recreation + transition to new window)!");

    vkDeviceWaitIdle(m_renderDeviceCtx->logicalDevice);
    m_swapchainMgr.recreateSwapchain(oldWindow, newWindow);

    updateCtx();

    return m_windowCtx.get();
}


void VkWindowManager::updateCtx() {
    m_windowCtx->surface                = m_coreResourcesMgr->getSurface();
    m_windowCtx->surfaceFormat          = m_swapchainMgr.getSurfaceFormat();
    m_windowCtx->swapchain              = m_swapchainMgr.getSwapChain();
    m_windowCtx->extent                 = m_swapchainMgr.getSwapChainExtent();
    m_windowCtx->presentMode            = m_swapchainMgr.getPresentMode();
    m_windowCtx->minImageCount          = m_swapchainMgr.getMinImageCount();
    m_windowCtx->images                 = m_swapchainMgr.getImages();
    m_windowCtx->imageViews             = m_swapchainMgr.getImageViews();
    m_windowCtx->framebuffers           = m_swapchainMgr.getFramebuffers();
    m_windowCtx->imageLayouts           = m_swapchainMgr.getImageLayouts();
    m_windowCtx->swapchainResourceID    = m_swapchainMgr.getSwapchainResourceID();
}
