# A list of helper scripts to plot hashmap benchmarks

The base of the benchmarking code is
```
.../bpf-next/tools/testing/selftests/bpf/bench bpf-hashmap-lookup
```
(not merged yet, see the lore [thread](https://lore.kernel.org/bpf/20230127181457.21389-1-aspsk@isovalent.com/)).

I am using these scripts to compare maps with original and patched hash
function `map_flags |= 0x02000`. Also maps output some debug info (`map_flags
|= 0x04000`). Pass your own `map_flags` if needed. Scripts also use `0x40`
flag to set seed to zero so that results are more predictable.

In order to get more-or-less stable results this is required to reduce noise,
e.g.,
```
#! /bin/bash

echo "0" | sudo tee /sys/devices/system/cpu/cpufreq/boost
echo off | sudo tee /sys/devices/system/cpu/smt/control

export LD_LIBRARY_PATH=~/src/bpf-next/tools/power/cpupower
~/src/bpf-next/tools/power/cpupower/cpupower frequency-set -f 3.50GHz
~/src/bpf-next/tools/power/cpupower/cpupower frequency-info

grep -H . /sys/devices/system/cpu/cpu*/topology/thread_siblings_list

cat /proc/cpuinfo | grep "^[c]pu MHz"
```
(This particular script works for AMD Ryzen 9 3950X.)

## run.bench.once.sh

This script just executes the bench once. Basically used to remind me how to
execute the bench. Just do
```
./run.bench.once.sh
```
and see what happens.

## run.bench-fullness.sh

A script which runs a series of ./bench benchmarks for different fullness of a
particular map (`max_entries`, `key_size`). For example, 
```
./run.bench-fullness.sh 65536 32
```
will benchmark maps with 6553, 13106, ..., 65536 elements (with
`max_entries=65536`) with key size 32. The results will be saved in
```
out/fullness/max_entries-65536-key_size-016
out/fullness/max_entries-65536-key_size-016.png
```
the former is txt, the latter is the picture plotted by [plot-it.py](../plot-it).

## run.bench-key-size.sh

A script which runs a series of ./bench benchmarks for different `key_sizes` of
a particular map (`max_entries`, `fullness`). For example, 
```
./run.bench-key-size.sh 65536 33
```
will benchmark map with `65536 * 0.33` elements (with
`max_entries=65536`) with key sizes 4, 8, ..., 64. The results will be saved in
```
out/key-size/max_entries-65536-fullness-33
out/key-size/max_entries-65536-fullness-33.png
```
the former is txt, the latter is the picture plotted by [plot-it.py](../plot-it).

## run.big-fullness.sh

Just run the `run.bench-fullness.sh` in a loop for different map and key sizes.
Takes a while, better to execute overnight.

## run.big-key-size.sh

Just run the `run.key-size.sh` in a loop for different map and key sizes.
Takes a while, better to execute overnight.
