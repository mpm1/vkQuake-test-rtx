#pragma once
#ifndef QK_ALLOCATOR
#define QK_ALLOCATOR

#include "vk_raytracedef.h"

void Alloc_Init(VkDevice device, VkPhysicalDevice pysicalDevice, VkInstance instance);
void Alloc_GetStructureMemoryRequirements(VkBindAccelerationStructureMemoryInfoKHR* memReqInfo, VkMemoryRequirements2* memReq);
VkDeviceMemory Alloc_AllocateMemory(VkMemoryAllocateInfo allocateInfo);

vk_accelerator_t Alloc_CreateAcceleration(VkAccelerationStructureCreateInfoKHR asCreateInfo);

vk_buffer_t Alloc_CreateBufferBase(VkBufferCreateInfo info, const VkMemoryPropertyFlags memUsage);
vk_buffer_t Alloc_CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, const VkMemoryPropertyFlags memUsage);

#endif // !QK_ALLOCATOR
