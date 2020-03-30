import os
import numpy as np

from cffi import FFI
ffi  = FFI()

ffi.cdef("""    
void render(int width, int height, float bg_r, float bg_g, float bg_b,
	int num_vertices, float* pUVs, float* pColors, int num_triangles, int* pIndices, float* pOutput);
""")

if os.name == 'nt':
	Native = ffi.dlopen('TextureGen.dll')
elif os.name == "posix":
	Native = ffi.dlopen('libTextureGen.so')

def Render(width, height, bg_col, UVs, Colors, Faces):
	num_vertices = len(UVs)
	num_faces = len(Faces)	
	pUV = ffi.cast("float *", UVs.__array_interface__['data'][0])
	pColor = ffi.cast("float *", Colors.__array_interface__['data'][0])
	pIndices = ffi.cast("int *", Faces.__array_interface__['data'][0])
	result = np.empty([height, width, 3], dtype=np.float32)
	pOutput = ffi.cast("float *", result.__array_interface__['data'][0])
	Native.render(width, height, bg_col[0], bg_col[1], bg_col[2], num_vertices, pUV, pColor, num_faces, pIndices, pOutput)
	return result

