#include <stdio.h>
#include <vector>
#include "renderer.h"
#include "shaders_packed.hpp"


VkShaderModule createShaderModule_from_spv(const void* bin, size_t bytes)
{
	const Context& ctx = Context::get_context();

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = bytes;
	createInfo.pCode = reinterpret_cast<const uint32_t*>(bin);
	VkShaderModule shaderModule;
	vkCreateShaderModule(ctx.device(), &createInfo, nullptr, &shaderModule);

	return shaderModule;
}


const static VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
const static size_t pixel_size = sizeof(float) * 4;

const Renderer& Renderer::get_renderer()
{
	static Renderer renderer;
	return renderer;
}

Renderer::Renderer()
{
	const Context& ctx = Context::get_context();

	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = format;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		vkCreateRenderPass(ctx.device(), &renderPassInfo, nullptr, &m_renderPass);
	}



	{
		m_vertShaderModule = createShaderModule_from_spv(s_vert_simple, sizeof(s_vert_simple));
		m_fragShaderModule = createShaderModule_from_spv(s_frag_simple, sizeof(s_frag_simple));

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		vkCreatePipelineLayout(ctx.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
	}

}

Renderer::~Renderer()
{
	const Context& ctx = Context::get_context();
	vkDestroyPipelineLayout(ctx.device(), m_pipelineLayout, nullptr);
	vkDestroyShaderModule(ctx.device(), m_fragShaderModule, nullptr);
	vkDestroyShaderModule(ctx.device(), m_vertShaderModule, nullptr);
	vkDestroyRenderPass(ctx.device(), m_renderPass, nullptr);
}

void Renderer::render(int width, int height, float bg_r, float bg_g, float bg_b,
	int num_vertices, float* pUVs, float* pColors, int num_triangles, int* pIndices, float* pOutput) const
{
	const Context& ctx = Context::get_context();

	DeviceBuffer bufUV(sizeof(float) * 2 * num_vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	bufUV.upload(pUVs);

	DeviceBuffer bufColors(sizeof(float) * 3 * num_vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	bufColors.upload(pColors);

	const VkBuffer vert_bufs[2] = { bufUV.buf(), bufColors.buf() };

	DeviceBuffer bufIndices(sizeof(int) * 3 * num_triangles, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	bufIndices.upload(pIndices);

	VkPipeline graphicsPipeline;
	{
		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = m_vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = m_fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		VkVertexInputBindingDescription bindingDescriptions[2] = {};
		bindingDescriptions[0] = {  };
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(float) * 2;
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		bindingDescriptions[1] = {  };
		bindingDescriptions[1].binding = 1;
		bindingDescriptions[1].stride = sizeof(float) * 3;
		bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription attributeDescriptions[2] = {};
		attributeDescriptions[0] = {  };
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = 0;

		attributeDescriptions[1] = {  };
		attributeDescriptions[1].binding = 1;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = 0;

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 2;
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions;
		vertexInputInfo.vertexAttributeDescriptionCount = 2;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport;
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)width;
		viewport.height = (float)height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = { (unsigned)width, (unsigned)height };

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_NONE;

		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = m_pipelineLayout;
		pipelineInfo.renderPass = m_renderPass;

		vkCreateGraphicsPipelines(ctx.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);

	}

	VkFramebuffer framebuffer;
	Texture fbTex(width, height, pixel_size, format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

	{
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &fbTex.view();
		framebufferInfo.width = width;
		framebufferInfo.height = height;
		framebufferInfo.layers = 1;
		vkCreateFramebuffer(ctx.device(), &framebufferInfo, nullptr, &framebuffer);
	}

	{
		NTimeCommandBuffer cmdBuf;

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_renderPass;
		renderPassInfo.framebuffer = framebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = { (unsigned)width, (unsigned)height };

		VkClearValue clearColor = { bg_r, bg_g, bg_b, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(cmdBuf.buf(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(cmdBuf.buf(), VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		VkDeviceSize offsets[2] = { 0, 0 };
		vkCmdBindVertexBuffers(cmdBuf.buf(), 0, 2, vert_bufs, offsets);
		vkCmdBindIndexBuffer(cmdBuf.buf(), bufIndices.buf(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(cmdBuf.buf(), num_triangles*3, 1, 0, 0, 0);
		vkCmdEndRenderPass(cmdBuf.buf());
	}

	std::vector<float> rgba(width*height * 4);
	fbTex.downloadTexture(rgba.data());

	for (size_t i = 0; i < width*height; i++)
	{
		pOutput[i * 3] = rgba[i * 4];
		pOutput[i * 3 + 1] = rgba[i * 4 + 1];
		pOutput[i * 3 + 2] = rgba[i * 4 + 2];
	}
	
	vkDestroyFramebuffer(ctx.device(), framebuffer, nullptr);
	vkDestroyPipeline(ctx.device(), graphicsPipeline, nullptr);

}