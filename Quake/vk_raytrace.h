#pragma once

#ifndef QK_VULKAN_RAYTRACE
#define QK_VULKAN_RAYTRACE

#define VK_ENABLE_BETA_EXTENSIONS
#define VK_USE_PLATFORM_WIN32_KHR

#include "vulkan/vulkan.h";

static VkPhysicalDeviceRayTracingPropertiesKHR vulkan_raytrace_properties;

// mpm1
// RayTracing
void VK_InitRayTracing(VkPhysicalDevice vulkan_physical_device);

#endif // !QK_VULKAN_RAYTRACE