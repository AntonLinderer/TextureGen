import numpy as np
import TextureGen as tg
from PIL import Image

bgCol = (127.0, 127.0, 127.0)

UVs = np.array([ [0.5, 0.25], [0.75, 0.75], [0.25, 0.75] ], dtype = np.float32)
Colors =  np.array([ [0.0, 255.0, 0.0], [255.0, 0.0, 0.0], [0.0, 0.0, 255.0]], dtype = np.float32)
Faces = np.array([ [ 0, 1, 2]], dtype = np.int32)

tex = tg.Render(1024, 768, bgCol, UVs, Colors, Faces)
tex = tex.astype(np.uint8)

img = Image.fromarray(tex, 'RGB')
img.save('output.png')

