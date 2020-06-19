#pragma once

#include "vk_raytrace.h"

static VKRayTraceBuilder raytraceBuilder;

/*
=================
RayTrace_Init
=================
*/
void RayTrace_Init(VkPhysicalDevice vulkan_physical_device, VkDevice vulkan_device, VkInstance vulkan_instance) {
	vulkan_raytrace_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_KHR;
	vkGetPhysicalDeviceProperties2(vulkan_physical_device, &vulkan_raytrace_properties);

	Alloc_Init(vulkan_device, vulkan_physical_device, vulkan_instance);

	raytraceBuilder.device = vulkan_device;
	raytraceBuilder.queueIndex = 0;
}

blas_t RayTrace_ConvertToBlas(qmodel_t *model) {
	// Acceleration Structure creation info.
	VkAccelerationStructureCreateGeometryTypeInfoKHR  asCreate = {
		.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
		.indexType = VK_INDEX_TYPE_UINT32,
		.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
		.maxPrimitiveCount = model->numsurfaces,
		.maxVertexCount = model->numvertexes,
		.allowsTransforms = VK_FALSE
	};

	// Build
	VkBufferDeviceAddressInfo indexAddressInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	indexAddressInfo.buffer = model->index_buffer;
	VkDeviceAddress indexAddress = vkGetBufferDeviceAddress(raytraceBuilder.device, &indexAddressInfo);

	VkBufferDeviceAddressInfo vertexAddressInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	vertexAddressInfo.buffer = model->vertex_buffer;
	VkDeviceAddress vertexAddress = vkGetBufferDeviceAddress(raytraceBuilder.device, &vertexAddressInfo);

	VkAccelerationStructureGeometryTrianglesDataKHR triangles = {
		.vertexFormat = asCreate.vertexFormat,
		.vertexData = vertexAddress,
		.vertexStride = sizeof(mvertex_t),
		.indexType = asCreate.indexType,
		.indexData = indexAddress,
		.transformData = VK_NULL_HANDLE
	};

	VkAccelerationStructureGeometryKHR asGeom = {
		.geometryType = asCreate.geometryType,
		.flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
		.geometry = {
			.triangles = triangles
		}
	};

	// Adding the Primitive	
	VkAccelerationStructureBuildOffsetInfoKHR offset = {
		.firstVertex = 0,
		.primitiveCount = asCreate.maxPrimitiveCount,
		.primitiveOffset = 0,
		.transformOffset = 0
	};

	// At the moment, add the single geometry as the BLAS
	blas_t blas = {
		.asGeom = asGeom,
		.asCreateGeometryInfo = asCreate,
		.asBuildOffsetInfo = offset
	};

	return blas;
}

void RayTrace_BuildBlas(qmodel_t** models, int count, VkBuildAccelerationStructureFlagsKHR flags) {
	int index;
	VkDeviceSize maxScratch = 0;
	VkResult err;
	qmodel_t *model;

	
	// TODO: Add support for 

	for (index = 0; index < count; ++index) {
		model = models[index];

		VkAccelerationStructureCreateInfoKHR asCreateInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
		asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		asCreateInfo.flags = flags;
		asCreateInfo.compactedSize = 0;
		asCreateInfo.maxGeometryCount = 1; // For now each blas only has 1 geometry contained within it. We will eventually need to change this.
		asCreateInfo.pGeometryInfos = &model->blas.asCreateGeometryInfo;

		model->blas.as = Alloc_CreateAcceleration(asCreateInfo);

		VkAccelerationStructureMemoryRequirementsInfoKHR memoryRequirementsInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR };
		memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR;
		memoryRequirementsInfo.accelerationStructure = model->blas.as.accel;
		memoryRequirementsInfo.buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;

		VkMemoryRequirements2 reqMem = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
		Alloc_GetStructureMemoryRequirements(&memoryRequirementsInfo, &reqMem);
		VkDeviceSize scratchSize = reqMem.memoryRequirements.size;

		model->blas.flags = flags;
		maxScratch = q_max(maxScratch, scratchSize);

		memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR;
		Alloc_GetStructureMemoryRequirements(&memoryRequirementsInfo, &reqMem);
	}

	vk_buffer_t scratchBuffer = Alloc_CreateBuffer(maxScratch, VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VkBufferDeviceAddressInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	bufferInfo.buffer = scratchBuffer.buffer;

	VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(raytraceBuilder.device, &bufferInfo);
}

void RayTrace_NewMap(void) {
	int			index;
	int blasCount = 0;
	qmodel_t*	model;
	qmodel_t*	blasModels[MAX_MODELS]; // TODO: Move to heap
	
	for (index = 1; index < MAX_MODELS; ++index) {
		model = cl.model_precache[index];
		if (!model || model->name[0] == '*' || model->vertex_buffer == NULL)
			continue;

		model->blas = RayTrace_ConvertToBlas(model);
		blasModels[blasCount++] = model;
	}

	RayTrace_BuildBlas(blasModels, blasCount, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
}