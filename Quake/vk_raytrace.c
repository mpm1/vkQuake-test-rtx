#pragma once

#include "vk_raytrace.h"

/*
=================
VK_InitRayTracing
=================
*/
void VK_InitRayTracing(VkPhysicalDevice vulkan_physical_device) {
	vulkan_raytrace_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_KHR;
	vkGetPhysicalDeviceProperties2(vulkan_physical_device, &vulkan_raytrace_properties);
}