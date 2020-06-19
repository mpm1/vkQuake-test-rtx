#pragma once

#ifndef QK_VULKAN_RAYTRACE_DEF
#define QK_VULKAN_RAYTRACE_DEF

#include "vulkan/vulkan.h"

#define RT_GET_DEVICE_PROC_ADDR(dev, entrypoint) { \
	rt##entrypoint = (PFN_vk##entrypoint)rtGetDeviceProcAddr(dev, "vk" #entrypoint); \
	if (rt##entrypoint == NULL) Sys_Error("vkGetDeviceProcAddr failed to find vk" #entrypoint); \
}

typedef struct vk_accelerator_s {
	VkAccelerationStructureKHR accel;
	VkDeviceMemory allocation;
} vk_accelerator_t;

typedef struct instance_s {
	uint32_t	blasId;
	uint32_t	instanceId;
	uint32_t	mask; // Should default to 0xFF
	float	transformMat[16]; // Should by default be the identity.
} instance_t;

// Our BLAS (Bottom Level Acceleration Structure)
typedef struct blas_s {
	vk_accelerator_t as;
	VkBuildAccelerationStructureFlagsKHR flags;
	VkAccelerationStructureGeometryKHR asGeom;
	VkAccelerationStructureCreateGeometryTypeInfoKHR asCreateGeometryInfo;
	VkAccelerationStructureBuildOffsetInfoKHR asBuildOffsetInfo;
} blas_t;

typedef struct vk_buffer_s {
	VkBuffer buffer;
	VkDeviceMemory allocation;
} vk_buffer_t;

typedef struct vk_raytracebuilkder_s {
	VkDevice device;
	uint32_t queueIndex;
} VKRayTraceBuilder;

#endif // !QK_VULKAN_RAYTRACE_DEF
