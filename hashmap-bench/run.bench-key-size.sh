#! /bin/bash -x

max_entries=${1:-65536}
fullness=${2:-50}
limit=${3:-64}

nr_entries=$((max_entries * fullness / 100))

out_dir="./out/key-size"
mkdir -p "$out_dir"

out=`readlink -f $out_dir/max_entries-$max_entries-fullness-$(printf '%03d' $fullness)`

bench=${bench:-`readlink -f ~/src/bpf-next/tools/testing/selftests/bpf/bench`}

do_bench() {
	local key_size="$1"
	local map_flags="$2"

	sudo $bench -d2 -a bpf-hashmap-lookup \
		--key_size=$key_size \
		--nr_entries=$nr_entries \
		--max_entries=$max_entries \
		--map_flags=$map_flags \
		--quiet \
		#
}

echo -n "hash_size" > "$out"
for key_size in `seq 4 4 $limit`; do
	echo -n " $key_size" >> "$out"
done
echo >> "$out"

echo -n orig_map >> "$out"
for key_size in `seq 4 4 $limit`; do
	M=`do_bench $key_size 0x00000040`
	echo -n " $M" >> "$out"
done
echo >> "$out"

echo -n new_map >> "$out"
for key_size in `seq 4 4 $limit`; do
	M=`do_bench $key_size 0x00002040`
	echo -n " $M" >> "$out"
done
echo >> "$out"

`readlink -f ../plot-it/plot-it.py` --xlabel "key_size (max_entries=$max_entries, fullness=$fullness%)" --ylabel "M ops / second" $out
