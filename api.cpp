#include "api.h"
#include "renderer.h"

void render(int width, int height, float bg_r, float bg_g, float bg_b,
	int num_vertices, float* pUVs, float* pColors, int num_triangles, int* pIndices, float* pOutput)
{
	const Renderer& renderer = Renderer::get_renderer();
	renderer.render(width, height, bg_r, bg_g, bg_b, num_vertices, pUVs, pColors, num_triangles, pIndices, pOutput);
}
