import matplotlib.pyplot as plt
from pathlib import Path
import os
import numpy as np
file = os.path.join(Path(__file__).parent, "grid.txt")

with open(file, "r") as file:
    lines = file.readlines()

for idx, line in enumerate(lines):
    lines[idx] = line.split()
    for id, s in enumerate(lines[idx]):
        lines[idx][id] = float(s)


def plot(ll, title, filename):
    array = np.array(ll)
    array = np.rot90(array)
    array = np.rot90(array)
    plt.imshow(array, cmap="viridis")
    plt.colorbar()
    plt.contourf(array, levels=20)
    plt.xlabel("X-axis")
    plt.ylabel("Y-axis")
    plt.title(title)
    out_file = os.path.join(Path(__file__).parent, filename)
    plt.savefig(out_file, dpi=300, bbox_inches="tight")
    plt.close()

plot(lines, "serial m=100", "raytracing")

