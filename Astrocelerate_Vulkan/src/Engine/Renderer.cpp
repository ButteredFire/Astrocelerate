/* Renderer.cpp - Renderer implementation.
*/


#include "Renderer.hpp"


Renderer::Renderer(VulkanContext &context):
    vulkInst(context.vulkanInstance), 
    vkContext(context) {

}


Renderer::~Renderer() {
    
};
