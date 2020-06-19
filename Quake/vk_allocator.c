#include "vk_allocator.h"

#include "SDL_vulkan.h"

static PFN_vkGetInstanceProcAddr rtGetInstanceProcAddr;
static PFN_vkGetDeviceProcAddr rtGetDeviceProcAddr;

static PFN_vkCreateAccelerationStructureKHR rtCreateAccelerationStructureKHR;
static PFN_vkGetAccelerationStructureMemoryRequirementsKHR rtGetAccelerationStructureMemoryRequirementsKHR;
static PFN_vkBindAccelerationStructureMemoryKHR rtBindAccelerationStructureMemoryKHR;
static PFN_vkCreateBuffer rtCreateBuffer;
static PFN_vkGetBufferMemoryRequirements2 rtGetBufferMemoryRequirements2;
static PFN_vkBindBufferMemory rtBindBufferMemory;

static VkDevice m_device;
static VkPhysicalDevice m_physicalDevice;
static VkPhysicalDeviceMemoryProperties m_memoryProperties;

//--------------------------------------------------------------------------------------------------
// Finding the memory type for memory allocation
// Memory function from nvvk
//
uint32_t getMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags properties)
{
	for (uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; i++)
	{
		if (((typeBits & (1 << i)) > 0) && (m_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	return ~0u;
}

void Alloc_Init(VkDevice device, VkPhysicalDevice physicalDevice, VkInstance instance) {
	rtGetInstanceProcAddr = SDL_Vulkan_GetVkGetInstanceProcAddr();
	rtGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)rtGetInstanceProcAddr(instance, "vkGetDeviceProcAddr");

	RT_GET_DEVICE_PROC_ADDR(device, CreateAccelerationStructureKHR);
	RT_GET_DEVICE_PROC_ADDR(device, GetAccelerationStructureMemoryRequirementsKHR);
	RT_GET_DEVICE_PROC_ADDR(device, BindAccelerationStructureMemoryKHR);
	RT_GET_DEVICE_PROC_ADDR(device, CreateBuffer);
	RT_GET_DEVICE_PROC_ADDR(device, GetBufferMemoryRequirements2);
	RT_GET_DEVICE_PROC_ADDR(device, BindBufferMemory);

	m_device = device;
	m_physicalDevice = physicalDevice;

	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &m_memoryProperties);
}

vk_accelerator_t Alloc_CreateAcceleration(VkAccelerationStructureCreateInfoKHR asCreateInfo) {
	vk_accelerator_t result;
	rtCreateAccelerationStructureKHR(m_device, &asCreateInfo, NULL, &result.accel);

	VkAccelerationStructureMemoryRequirementsInfoKHR memInfo = { 
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR 
	};
	memInfo.accelerationStructure = result.accel;
	memInfo.buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
	memInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR;
	VkMemoryRequirements2 memoryRequirements = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
	Alloc_GetStructureMemoryRequirements(&memInfo, &memoryRequirements);

	VkMemoryAllocateFlagsInfo memFlagInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR };
	memFlagInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

	VkMemoryAllocateInfo memoryAlloc = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memoryAlloc.allocationSize = memoryRequirements.memoryRequirements.size;
	memoryAlloc.memoryTypeIndex = getMemoryType(memoryRequirements.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	result.allocation = Alloc_AllocateMemory(memoryAlloc);

	// Bind the the acceleration structure to memory
	VkBindAccelerationStructureMemoryInfoKHR bind = { VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR };
	bind.accelerationStructure = result.accel;
	bind.memory = result.allocation;
	bind.memoryOffset = 0;
	rtBindAccelerationStructureMemoryKHR(m_device, 1, &bind);

	return result;
}

VkDeviceMemory Alloc_AllocateMemory(VkMemoryAllocateInfo allocateInfo) {
	VkDeviceMemory memory;
	vkAllocateMemory(m_device, &allocateInfo, NULL, &memory);

	return memory;
}

void Alloc_GetStructureMemoryRequirements(VkBindAccelerationStructureMemoryInfoKHR* memReqInfo, VkMemoryRequirements2* memReq) {
	rtGetAccelerationStructureMemoryRequirementsKHR(m_device, memReqInfo, memReq);
}

vk_buffer_t Alloc_CreateBufferBase(VkBufferCreateInfo info, const VkMemoryPropertyFlags memUsage) {
	vk_buffer_t resultBuffer;

	rtCreateBuffer(m_device, &info, NULL, &resultBuffer.buffer);

	VkMemoryRequirements2 memReqs = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
	VkMemoryDedicatedRequirements dedicatedReqs = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS };
	VkBufferMemoryRequirementsInfo2 bufferReqs = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2 };

	bufferReqs.buffer = resultBuffer.buffer;
	memReqs.pNext = &dedicatedReqs;
	rtGetBufferMemoryRequirements2(m_device, &bufferReqs, &memReqs);

	// Device Address
	VkMemoryAllocateFlagsInfo memFlagsInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO };
	if (info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
		memFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
	}

	VkMemoryAllocateInfo memAlloc = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memAlloc.allocationSize = memReqs.memoryRequirements.size;
	memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryRequirements.memoryTypeBits, memUsage);
	memAlloc.pNext = &memFlagsInfo;
	resultBuffer.allocation = Alloc_AllocateMemory(memAlloc);

	// TODO: CHECK MEMORY

	rtBindBufferMemory(m_device, resultBuffer.buffer, resultBuffer.allocation, 0);

	return resultBuffer;
}

vk_buffer_t Alloc_CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, const VkMemoryPropertyFlags memUsage) {
	VkBufferCreateInfo info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	info.size = size;
	info.usage = usage;

	return Alloc_CreateBufferBase(info, memUsage);
}
