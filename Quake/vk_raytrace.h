// mpm1
// RayTracing

#pragma once

#ifndef QK_VULKAN_RAYTRACE
#define QK_VULKAN_RAYTRACE

#include "quakedef.h"
#include "vk_raytracedef.h"
#include "vk_allocator.h"

VkPhysicalDeviceRayTracingPropertiesKHR vulkan_raytrace_properties;

void RayTrace_Init(VkPhysicalDevice vulkan_physical_device, VkDevice vulkan_device, VkInstance vulkan_instance);
blas_t RayTrace_ConvertToBlas(qmodel_t* model);
void RayTrace_BuildBlas(qmodel_t** models, int count, VkBuildAccelerationStructureFlagsKHR flags);

void RayTrace_NewMap(void);

#endif // !QK_VULKAN_RAYTRACE