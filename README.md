# TextureGen

CV folks sometimes have a triangle mesh with a reconstructed color value for each of the vertices, 
along with a UV coordinate pointing to the texture plane, and they want to project the per-vertex color values into
the texture, so they can further refine it.

I see that this work is sometime done with a software rasterizer or even Python, due to the lack of CG programming skill.
Even for CG programmers, off-screen rendering is not a typical use-case, and can be tricky.

In this repo, I provide a simple Vulkan based off-screen rasterizer, which can be used from Python. The geometry data can
be passed in as numpy NDArrays, to make it friendly to CV folks.

See python/test.py for how to use it.



