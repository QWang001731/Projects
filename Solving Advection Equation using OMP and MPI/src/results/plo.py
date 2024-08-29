import matplotlib.pyplot as plt
from pathlib import Path
import numpy as np
import os

def plot(ll, title, filename):
    array = np.array(ll)
    plt.imshow(array, cmap="viridis")
    plt.colorbar()
    plt.contourf(array, levels=20)
    plt.xlabel("X-axis")
    plt.ylabel("Y-axis")
    plt.title(title)
    out_file = os.path.join(Path(__file__).parent, filename)
    plt.savefig(out_file, dpi=300, bbox_inches="tight")
    plt.close()
    return

def read_plot(file_name0,file_name1):
    para_file_path = os.path.join(Path(__file__).parent, file_name0)
    with open(para_file_path, 'r') as file:
        lines0 = file.readlines()
        for idx, s in enumerate(lines0):
            lines0[idx] = s.split()
            for id, s in enumerate(lines0[idx]):
                lines0[idx][id] = float(s)


    para_file_path = os.path.join(Path(__file__).parent, file_name1)
    with open(para_file_path, 'r') as file:
        lines1 = file.readlines()
        for idx, s in enumerate(lines1):
            lines1[idx] = s.split()
            for id, s in enumerate(lines1[idx]):
                lines1[idx][id] = float(s)
    
    lines = lines0 + lines1

    plot(lines,title=file_name0[:-4],filename=file_name0[:-4]+".jpg")

read_plot("init0.txt","init1.txt")
read_plot("middle0.txt","middle1.txt")
read_plot("end0.txt","end1.txt")