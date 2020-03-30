#pragma once

#include "volk.h"
#include <stdio.h>
#include <stdlib.h>

class Context
{
public:
	static const Context& get_context();

	const VkInstance& instance() const { return m_instance; }
	const VkPhysicalDevice& physicalDevice() const { return m_physicalDevice; }
	const VkDevice& device() const { return m_device; }
	const VkQueue& queue() const { return m_graphicsQueue; }
	const VkCommandPool& commandPool() const { return m_commandPool_graphics; }

private:
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkInstance m_instance;
	VkPhysicalDevice m_physicalDevice;
	uint32_t m_graphicsQueueFamily;
	float m_queuePriority;
	VkDevice m_device;
	VkQueue m_graphicsQueue;
	VkCommandPool m_commandPool_graphics;

	bool _init_vulkan();
	Context();
	~Context();
};


class NTimeCommandBuffer
{
public:
	const VkCommandBuffer& buf() const { return m_buf; }

	NTimeCommandBuffer(size_t n = 1);
	~NTimeCommandBuffer();

private:
	VkCommandBuffer m_buf;
	size_t m_n;
};


class Buffer
{
public:
	VkDeviceSize size() const { return m_size; }
	const VkBuffer& buf() const { return m_buf; }
	const VkDeviceMemory& memory() const { return m_mem; }

protected:
	VkDeviceSize m_size;
	VkBuffer m_buf;
	VkDeviceMemory m_mem;

	Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags);
	virtual ~Buffer();

};

class UploadBuffer : public Buffer
{
public:
	UploadBuffer(VkDeviceSize size);
	virtual ~UploadBuffer();

	void upload(const void* hdata);
	void zero();

};

class DownloadBuffer : public Buffer
{
public:
	DownloadBuffer(VkDeviceSize size);
	virtual ~DownloadBuffer();

	void download(void* hdata);
};

class DeviceBuffer : public Buffer
{
public:
	DeviceBuffer(VkDeviceSize size, VkBufferUsageFlags usage);
	virtual ~DeviceBuffer();

	void upload(const void* hdata);
	void zero();
	void download(void* hdata, VkDeviceSize begin = 0, VkDeviceSize end = (VkDeviceSize)(-1));

};


class Texture
{
public:
	int width() const { return m_width; }
	int height() const { return m_height; }
	int pixel_size() const { return m_pixel_size; }
	const VkFormat& format() const { return m_format; }
	const VkImage& image() const { return m_image; }
	const VkDeviceMemory& memory() const { return m_mem; }
	const VkImageView& view() const { return m_view; }

	Texture(int width, int height, int pixel_size, VkFormat format, VkImageAspectFlags aspectFlags, VkImageUsageFlags usage);
	~Texture();

	void uploadTexture(const void* hdata);
	void downloadTexture(void* hdata);

private:
	int m_width;
	int m_height;
	int m_pixel_size;
	VkFormat m_format;

	VkImage m_image;
	VkDeviceMemory m_mem;
	VkImageView m_view;

};


class Sampler
{
public:
	Sampler();
	~Sampler();

	const VkSampler& sampler() const { return m_sampler; }

private:
	VkSampler m_sampler;
};
