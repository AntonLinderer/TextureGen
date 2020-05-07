#include "context.h"
#include <memory.h>
#include <vector>

const Context& Context::get_context()
{
	static Context ctx;
	return ctx;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	printf("validation layer: %s\n", pCallbackData->pMessage);
	return VK_FALSE;
}

bool Context::_init_vulkan()
{
	if (volkInitialize() != VK_SUCCESS) return false;

	{
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "TextureGen";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 1, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 1, 0);
		appInfo.apiVersion = VK_API_VERSION_1_1;

		const char* name_extensions[] = {
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};

		char str_validationLayers[] = "VK_LAYER_KHRONOS_validation";
		const char* validationLayers[] = { str_validationLayers };

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		debugCreateInfo = {};
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = debugCallback;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = 1;
		createInfo.ppEnabledExtensionNames = name_extensions;

#ifdef _DEBUG
		createInfo.enabledLayerCount = 1;
		createInfo.ppEnabledLayerNames = validationLayers;
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#endif

		if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) return false;

#ifdef _DEBUG
		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
		vkCreateDebugUtilsMessengerEXT(m_instance, &debugCreateInfo, nullptr, &m_debugMessenger);
#endif

	}
	volkLoadInstance(m_instance);

	m_physicalDevice = VK_NULL_HANDLE;
	{
		// select physical device
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
		std::vector<VkPhysicalDevice> ph_devices(deviceCount);
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, ph_devices.data());
		m_physicalDevice = ph_devices[0];
	}

	m_graphicsQueueFamily = (uint32_t)(-1);
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilyCount; i++)
			if (m_graphicsQueueFamily == (uint32_t)(-1) && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) m_graphicsQueueFamily = i;
	}

	// logical device/queue
	m_queuePriority = 1.0f;

	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = m_graphicsQueueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &m_queuePriority;

		const char* name_extensions[] = {
			VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
		};

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = &queueCreateInfo;
		createInfo.queueCreateInfoCount = 1;
		createInfo.enabledExtensionCount = 1;
		createInfo.ppEnabledExtensionNames = name_extensions;

		if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) return false;
	}

	vkGetDeviceQueue(m_device, m_graphicsQueueFamily, 0, &m_graphicsQueue);

	{
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = m_graphicsQueueFamily;
		vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool_graphics);
	}

	return true;
}


Context::Context()
{
	if (!_init_vulkan()) exit(0);
}

Context::~Context()
{
#ifdef _DEBUG
	vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
#endif
	vkDestroyCommandPool(m_device, m_commandPool_graphics, nullptr);
	vkDestroyDevice(m_device, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}

NTimeCommandBuffer::NTimeCommandBuffer(size_t n) : m_n(n)
{
	const Context& ctx = Context::get_context();

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = ctx.commandPool();
	allocInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(ctx.device(), &allocInfo, &m_buf);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (n == 1)
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	else
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	vkBeginCommandBuffer(m_buf, &beginInfo);
}

NTimeCommandBuffer::~NTimeCommandBuffer()
{
	const Context& ctx = Context::get_context();

	vkEndCommandBuffer(m_buf);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_buf;

	for (size_t i = 0; i < m_n; i++)
		vkQueueSubmit(ctx.queue(), 1, &submitInfo, 0);

	vkQueueWaitIdle(ctx.queue());

	vkFreeCommandBuffers(ctx.device(), ctx.commandPool(), 1, &m_buf);
}

Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags)
{
	if (size == 0) return;
	m_size = size;

	const Context& ctx = Context::get_context();

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkCreateBuffer(ctx.device(), &bufferCreateInfo, nullptr, &m_buf);

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(ctx.device(), m_buf, &memRequirements);

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(ctx.physicalDevice(), &memProperties);

	uint32_t memoryTypeIndex = VK_MAX_MEMORY_TYPES;
	for (uint32_t k = 0; k < memProperties.memoryTypeCount; k++)
	{
		if ((memRequirements.memoryTypeBits & (1 << k)) == 0) continue;
		if ((flags & memProperties.memoryTypes[k].propertyFlags) == flags)
		{
			memoryTypeIndex = k;
			break;
		}
	}

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

	vkAllocateMemory(ctx.device(), &memoryAllocateInfo, nullptr, &m_mem);
	vkBindBufferMemory(ctx.device(), m_buf, m_mem, 0);
}

Buffer::~Buffer()
{
	const Context& ctx = Context::get_context();
	vkDestroyBuffer(ctx.device(), m_buf, nullptr);
	vkFreeMemory(ctx.device(), m_mem, nullptr);
}

UploadBuffer::UploadBuffer(VkDeviceSize size) :
	Buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {	}

UploadBuffer::~UploadBuffer() {}

void UploadBuffer::upload(const void* hdata)
{
	if (m_size == 0) return;

	const Context& ctx = Context::get_context();

	void* data;
	vkMapMemory(ctx.device(), m_mem, 0, m_size, 0, &data);
	memcpy(data, hdata, m_size);
	vkUnmapMemory(ctx.device(), m_mem);
}

void UploadBuffer::zero()
{
	if (m_size == 0) return;

	const Context& ctx = Context::get_context();

	void* data;
	vkMapMemory(ctx.device(), m_mem, 0, m_size, 0, &data);
	memset(data, 0, m_size);
	vkUnmapMemory(ctx.device(), m_mem);
}

DownloadBuffer::DownloadBuffer(VkDeviceSize size) :
	Buffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT) { }

DownloadBuffer::~DownloadBuffer() {}

void DownloadBuffer::download(void* hdata)
{
	if (m_size == 0) return;

	const Context& ctx = Context::get_context();

	void* data;
	vkMapMemory(ctx.device(), m_mem, 0, m_size, 0, &data);
	memcpy(hdata, data, m_size);
	vkUnmapMemory(ctx.device(), m_mem);
}

DeviceBuffer::DeviceBuffer(VkDeviceSize size, VkBufferUsageFlags usage) :
	Buffer(size, usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {}

DeviceBuffer::~DeviceBuffer() {}

void DeviceBuffer::upload(const void* hdata)
{
	UploadBuffer staging_buf(m_size);
	staging_buf.upload(hdata);

	{
		NTimeCommandBuffer cmdBuf;

		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = m_size;
		vkCmdCopyBuffer(cmdBuf.buf(), staging_buf.buf(), m_buf, 1, &copyRegion);
	}
}

void DeviceBuffer::zero()
{
	UploadBuffer staging_buf(m_size);
	staging_buf.zero();

	{
		NTimeCommandBuffer cmdBuf;

		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = m_size;
		vkCmdCopyBuffer(cmdBuf.buf(), staging_buf.buf(), m_buf, 1, &copyRegion);
	}
}

void DeviceBuffer::download(void* hdata, VkDeviceSize begin, VkDeviceSize end)
{
	if (end > m_size) end = m_size;
	if (end <= begin) return;

	DownloadBuffer staging_buf(end - begin);

	{
		NTimeCommandBuffer cmdBuf;

		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = begin;
		copyRegion.dstOffset = 0;
		copyRegion.size = end - begin;
		vkCmdCopyBuffer(cmdBuf.buf(), m_buf, staging_buf.buf(), 1, &copyRegion);
	}

	staging_buf.download(hdata);
}

Texture::Texture(int width, int height, int pixel_size, VkFormat format, VkImageAspectFlags aspectFlags, VkImageUsageFlags usage)
{
	m_width = width;
	m_height = height;
	m_pixel_size = pixel_size;
	m_format = format;
	if (width == 0 || height == 0) return;

	const Context& ctx = Context::get_context();

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkCreateImage(ctx.device(), &imageInfo, nullptr, &m_image);

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(ctx.device(), m_image, &memRequirements);

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(ctx.physicalDevice(), &memProperties);

	uint32_t memoryTypeIndex = VK_MAX_MEMORY_TYPES;
	for (uint32_t k = 0; k < memProperties.memoryTypeCount; k++)
	{
		if ((memRequirements.memoryTypeBits & (1 << k)) == 0) continue;
		if ((VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT & memProperties.memoryTypes[k].propertyFlags) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		{
			memoryTypeIndex = k;
			break;
		}
	}

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = memoryTypeIndex;

	vkAllocateMemory(ctx.device(), &allocInfo, nullptr, &m_mem);
	vkBindImageMemory(ctx.device(), m_image, m_mem, 0);

	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = m_image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;
	vkCreateImageView(ctx.device(), &createInfo, nullptr, &m_view);
}

Texture::~Texture()
{
	if (m_width == 0 || m_height == 0) return;
	const Context& ctx = Context::get_context();
	vkDestroyImageView(ctx.device(), m_view, nullptr);
	vkDestroyImage(ctx.device(), m_image, nullptr);
	vkFreeMemory(ctx.device(), m_mem, nullptr);
}

void Texture::uploadTexture(const void* hdata)
{
	if (m_width == 0 || m_height == 0) return;
	UploadBuffer staging_buf(m_width*m_height*m_pixel_size);
	staging_buf.upload(hdata);

	{
		NTimeCommandBuffer cmdBuf;

		{
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = m_image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			vkCmdPipelineBarrier(
				cmdBuf.buf(),
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}

		{


			VkBufferImageCopy region = {};
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.layerCount = 1;
			region.imageExtent = {
				(uint32_t)m_width,
				(uint32_t)m_height,
				1
			};

			vkCmdCopyBufferToImage(
				cmdBuf.buf(),
				staging_buf.buf(),
				m_image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&region
			);
		}

		{

			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = m_image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				cmdBuf.buf(),
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

		}
	}
}

void Texture::downloadTexture(void* hdata)
{
	if (m_width == 0 || m_height == 0) return;
	DownloadBuffer staging_buf(m_width*m_height*m_pixel_size);
	
	{
		NTimeCommandBuffer cmdBuf;

		{
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = m_image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(
				cmdBuf.buf(),
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}


		{

			VkBufferImageCopy region = {};
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.layerCount = 1;
			region.imageExtent = {
				(uint32_t)m_width,
				(uint32_t)m_height,
				1
			};

			vkCmdCopyImageToBuffer( 
				cmdBuf.buf(),
				m_image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				staging_buf.buf(),
				1,
				&region
			);
		}

	}
	staging_buf.download(hdata);

}


Sampler::Sampler()
{
	const Context& ctx = Context::get_context();
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	vkCreateSampler(ctx.device(), &samplerInfo, nullptr, &m_sampler);
}

Sampler::~Sampler()
{
	const Context& ctx = Context::get_context();
	vkDestroySampler(ctx.device(), m_sampler, nullptr);
}

