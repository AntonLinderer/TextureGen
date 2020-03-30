#pragma once

#include "context.h"

class Renderer
{
public:
	static const Renderer& get_renderer();

	void render(int width, int height, float bg_r, float bg_g, float bg_b, 
		int num_vertices, float* pUVs, float* pColors, int num_triangles, int* pIndices, float* pOutput) const;


private:
	Renderer();
	~Renderer();

	VkRenderPass m_renderPass;
	VkShaderModule m_vertShaderModule;
	VkShaderModule m_fragShaderModule;

	VkPipelineLayout m_pipelineLayout;

};

