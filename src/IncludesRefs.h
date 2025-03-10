/* Includes.h: Used only as a handy reference. DO NOT ACTUALLY INCLUDE THIS!
*/

#pragma once

// GLFW & Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

// Dear ImGui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

// C++ STLs
#include <iostream>
#include <string>
#include <vector>

// Other
#include <Constants.h>
