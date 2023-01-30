#! /bin/python3

import matplotlib.pyplot as plt
from sys import exit

def input(filename):

        lines = open(filename).readlines()

        x = []
        for line in lines:
                elements = line.split()
                x.append([elements[0], [float(y) for y in elements[1:]]])

        return x

def plot_me(input_filename, output_filename='', only=[], xlabel="X axis", ylabel="Y axis"):

        x = input(input_filename)
        x, yy = x[0], x[1:]
        fig, ax = plt.subplots()

        print(x, yy)

        fig.set_figheight(5)
        fig.set_figwidth(10)

        YMax = max([max(y[1]) for y in yy]) + 5

        for y in yy:
                if (not only) or (y[0] in only):
                        print("plotting")
                        ax.plot(x[1], y[1], label=y[0])
                        ax.set_ylim(ymin=0, ymax=YMax)
                else:                     
                        print(f"not plotting y={y[0]}")

        ax.set_xlabel(xlabel)
        ax.set_ylabel(ylabel)
        ax.legend()

        if not output_filename:
                output_filename = input_filename + '.png'

        plt.savefig(output_filename, format='png', bbox_inches='tight', pad_inches=0.05)

import argparse
parser = argparse.ArgumentParser()
parser.add_argument('--xlabel', type=str, help='X axis label, default is "Input size (bytes)"', default='Input size (bytes)')
parser.add_argument('--ylabel', type=str, help='Y axis label, default is "Y"', default='Y label')
parser.add_argument('names', type=str, nargs='+', help='A list of files to process')
args = parser.parse_args()

for name in args.names:
	plot_me(name, xlabel=args.xlabel, ylabel=args.ylabel)
