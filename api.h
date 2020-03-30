#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define TEXTURE_GEN_API __declspec(dllexport)
#else
#define TEXTURE_GEN_API 
#endif

extern "C"
{
	TEXTURE_GEN_API void render(int width, int height, float bg_r, float bg_g, float bg_b,
		int num_vertices, float* pUVs, float* pColors, int num_triangles, int* pIndices, float* pOutput);
}
