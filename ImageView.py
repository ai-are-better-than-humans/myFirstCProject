from matplotlib.pyplot import imshow, show
import numpy as np


f = open(r"<Desktop Location>\Desktop\img_temp\pixels.txt", "r")
f2 = open(r"<Desktop Location>\Desktop\img_temp\info.txt", "r")

newvals = [int(x) for x in f.readline().split(',') if x != '\n']
info = [int(x) for x in f2.readline().split(',') if x != '\n']

width = info[0]
height = info[1]
bpx = info[2]

newvals = np.array(newvals).reshape((height, width, bpx))
imshow(newvals)
show()
