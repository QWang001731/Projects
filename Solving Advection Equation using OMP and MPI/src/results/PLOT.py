import matplotlib.pyplot as plt
from pathlib import Path
import numpy as np
import os
import matplotlib.animation as animation
from itertools import islice
from decimal import Decimal
# hard code N, NT values
N = 4000
NT = 20000
lines = []
lines1 = []
#serial_file_path = os.path.join(Path(__file__).parent, "data_serial.txt")
"""with open(serial_file_path, 'r') as file:
    lines = file.readlines()
    for idx, s in enumerate(lines):
        lines[idx] = s.split()
        for id, s in enumerate(lines[idx]):
            lines[idx][id] = float(s)"""
"""
with open("lax0.txt", "r") as file1, open("lax1.txt", "r") as file2:
    content = file1.read() + file2.read()

with open("lax.txt", "w") as combined_file:
    combined_file.write(content)"""

para_file_path = os.path.join(Path(__file__).parent, "middle.txt")
with open(para_file_path, 'r') as file:
    lines = file.readlines()
    for idx, s in enumerate(lines):
        lines[idx] = s.split()
        for id, s in enumerate(lines[idx]):
            lines[idx][id] = float(s)



#print(lines1[5000][2000]==lines1[9000][2000])
"""
line0 = lines[:400]
lax0 = lines[400:800]
lax1 = lines[800:1200]
first_order0 = lines[1200:1600]
first_order1 = lines[1600:2000]
second_order0 = lines[2000:2400]
second_order1 = lines[2400:2800]

line0_para = lines1[:400]
lax0_para = lines1[400:800]
lax1_para = lines1[800:1200]
first_order0_para = lines1[1200:1600]
first_order1_para = lines1[1600:2000]
second_order0_para = lines1[2000:2400]
second_order1_para = lines1[2400:2800]
"""

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

plot(lines[0:4000], " Lax shared16cores at t = 0", "Lax_shared16core_init")
plot(lines[4000:8000], "Lax shared16cores at t = 0.5", "Lax_shared16coremiddle")
plot(lines[8000:12000], "Lax shared16cores  solver at t = 1", "Lax_shared16coreT")

"""
plot(line0, "Initialized_distribution", "init")
plot(lax0, "Serial_Lax_at_t_=_T/2","lax0")

plot(lax1, "Serial_Lax_at_t_=_T","lax1")

plot(first_order0, "Serial First order at t = T/2","first_order0")
plot(first_order1, "Serial First order at t = T","first_order1")

plot(second_order0, "Serial second order at t = T/2","second_order0")
plot(second_order1, "Serial second order at t = T","second_order1")

plot(line0_para, "Initialized distribution","init_para")
plot(lax0_para, "Para Lax  at t = T/2","lax0_para")
plot(lax1_para, "Para Lax  at t = T","lax1_para")

plot(first_order0_para, "Para First order at t = T/2","first_order0_para")
plot(first_order1_para, "Para First order at t = T","first_order1_para")

plot(second_order0_para, "Para second order at t = T/2","second_order0_para")
plot(second_order1_para, "Para second order at t = T","second_order1_para")
"""

