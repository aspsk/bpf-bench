#! /bin/python3

import matplotlib.pyplot as plt
from sys import exit

def input(filename):

        lines = open(filename).readlines()

        x = []
        for line in lines:
                elements = line.split()
                x.append([elements[0], [int(y) for y in elements[1:]]])

        return x

def plot_me(input_filename, output_filename='', log=False, only=[], xlabel="Input size (bytes)"):

        x = input(input_filename)
        x, yy = x[0], x[1:]
        fig, ax = plt.subplots()

        print(x, yy)

        fig.set_figheight(5)
        fig.set_figwidth(10)

        for y in yy:
                if (not only) or (y[0] in only):
                        print("plotting")
                        ax.plot(x[1], y[1], label=y[0])
                else:                     
                        print(f"not plotting y={y[0]}")

        if log:
                ax.set_xscale('log')
        ax.set_xlabel(xlabel)
        ax.set_ylabel('CPU cycles')
        ax.legend()

        if not output_filename:
                output_filename = input_filename + '.png'

        plt.savefig(output_filename, format='png', bbox_inches='tight', pad_inches=0.05)

from os import system

def benchmark(funcs=(), sizes=(1, 1, 16), save='example.txt'):
        start, step, end = sizes
        system(f"sudo rm -f {save}")
        system(f"echo -n 'hash_size ' > {save}")
        system(f"seq -s' ' {start} {step} {end} >> {save}")
        for func in funcs:
                system(f'sudo ./__bench.sh {func} {start} {step} {end} {save}')

def benchmark_und_plot(funcs, sizes, name, xlabel):
        benchmark(funcs=funcs, sizes=sizes, save=f"{name}.txt")
        plot_me(f"{name}.txt", output_filename=f"{name}.png", xlabel=xlabel)

import argparse
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--hash', type=str, help='A list of hash function names, e.g., jhash,xxh3,bpf')
parser.add_argument('--range', type=str, help='The hash input range: start,step,end, e.g., 1,1,32 or 4,4,256')
parser.add_argument('--name', type=str, help='Output files prefix, e.g., --name=example would generate example.txt and example.png')
parser.add_argument('--xlabel', type=str, help='X axis label, default is "Input size (bytes)"', default='Input size (bytes)')
args = parser.parse_args()

funcs = args.hash.split(',')
for f in funcs:
        supported = ['none', 'bpf', 'test', 'jhash', 'jhash2', 'xxh3', 'xxh32', 'xxh64']
        if f not in supported:
                sys.exit(f"bad hash function '{f}'; supported functions: {supported}")

range_list = list(map(int, args.range.split(',')))
if len(range_list) != 3 or any([x <= 0 for x in range_list]):
                sys.exit(f"bad range list {range_list}, should be <start>,<step>,<end>")

benchmark_und_plot(funcs, range_list, name=args.name, xlabel=args.xlabel)
