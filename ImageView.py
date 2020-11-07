from matplotlib.pyplot import imshow, show
import numpy as np
from sys import argv

pt1 = r"C:\Users\mlfre\OneDrive\Desktop\img_temp\pixels.txt"
pt2 = r"C:\Users\mlfre\OneDrive\Desktop\img_temp\info.txt"
#f = open(input("pix:\n"), "r")
#f2 = open(input("info:\n"), "r")
f = open(pt1, "r")
f2 = open(pt2, "r")
newvals = [int(x) for x in f.readline().split(',') if x != '\n']

info = [int(x) for x in f2.readline().split(',') if x != '\n']
width = info[0]
height = info[1]
bpx = info[2]

newvals = np.array(newvals).reshape((height, width, bpx))
imshow(newvals)
show()