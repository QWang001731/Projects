import sys
from Emulator import CPU
import argparse
import random
LEN_OF_WORD = 32
DATA_SZ = 8
'''
Assumption:  a 32 bit system; matrix is stored in row major.
'''

parser = argparse.ArgumentParser(description="Cache Emulator Commandline")
parser.add_argument("-c", type=int, default=65556, help="cache size")
parser.add_argument("-b", type=int, default=64, help="cache block size")
parser.add_argument("-n", type=int, default=2, help="n-way associatity.")
parser.add_argument("-r", type=str, default="LRU", help="Replacement policy.")
parser.add_argument("-a", type=str, default="mxm_block", help="Algorithm")
parser.add_argument("-d", type=int, default=480, help="data structure size")
parser.add_argument('-p', action='store_true', help='Enable printing')
parser.add_argument('-f', type=int, default=32, help="matrix block size")


args = parser.parse_args()
associatity = int(args.n)
block_num = int(args.c/args.b)
num_of_sets = int(block_num/associatity)


cache = dict()
cache["policy"] = args.r
cache["associtivity"] = int(args.n)
cache["size"] = int(args.c)
cache["block_size"] = int(args.b)
cache["write_mode"] = "write_back"

if args.a == "daxpy":
    data_size = args.d
else:
    data_size = args.d * args.d
print("data_size:", data_size)

A = [1 for i in range(data_size)]
B = [1 for i in range(data_size)]
D = 3
b = args.f

ram_size = DATA_SZ * data_size * 3
myCpu = CPU(D, b, cache, A, B)


if args.a == "daxpy":
    R = myCpu.Daxpy()
elif args.a == "mxm_block":
    R = myCpu.DgemmBlock()
elif args.a == "mxm":
    R = myCpu.Dgemm()
else:
    print("Unknown algorithm")

instruction_num = myCpu.counter["instruction_num"]
read_misses = myCpu.counter["read_misses"]
read_hits = myCpu.counter["read_hits"]
write_hits = myCpu.counter["write_hits"]
write_misses = myCpu.counter["write_misses"]

read_miss_rate = read_misses/(read_misses + read_hits)
write_miss_rate = write_misses/(write_misses + write_hits)


if args.p:
    print("the calculated result is ", R)

print(f'''INPUTS==========================================
Ram Size=\t\t\t{ram_size}bytes\nCache Size=\t\t\t{args.c}bytes\nBlock Size=\
\t\t\t{args.b}bytes\nTotal Bolcks in Cache=\t\t{block_num}\nAssociativity=\
\t\t\t{args.n}\nNumber of Sets=\t\t\t{num_of_sets}\nRepla\
cement Policy=\t\t{args.r}\nAlgorithm=\t\t\t{args.a}\nMXM Blocking Factor=\t\
\t{args.f}\nMatrix or Vector dimension=\t{args.d}\n
RESULTS==========================================\n
Instruction Counts:\t\t{instruction_num}
Read hits:\t\t\t{read_hits}
Read misses:\t\t\t{read_misses}
Read miss rate:\t\t\t{read_miss_rate*100:.2f}%
Write hits:\t\t\t{write_hits}
Write misses:\t\t\t{write_misses}
Write miss rate:\t\t{write_miss_rate*100:.2f}%
''')
