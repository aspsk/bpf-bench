max_entries=${1:-65536}
key_size=${2:-16}

out_dir="./out/fullness"
mkdir -p "$out_dir"

out=`readlink -f $out_dir/max_entries-$max_entries-key_size-$(printf '%03d' $key_size)`

bench=${bench:-`readlink -f ~/src/bpf-next/tools/testing/selftests/bpf/bench`}

do_bench() {
	local nr_entries="$1"
	local map_flags="$2"

	sudo $bench -d2 -a bpf-hashmap-lookup \
		--key_size=$key_size \
		--nr_entries=$nr_entries \
		--max_entries=$max_entries \
		--map_flags=$map_flags \
		--nr_loops=2000000 \
		--quiet \
		#
}

echo -n "#elements(%)" > "$out"
for pp in `seq 1 1 10`; do
	echo -n " $((pp * 10))" >> "$out"
done
echo >> "$out"

echo -n orig_map >> "$out"
for pp in `seq 1 1 10`; do
	M=`do_bench $(((max_entries * pp) / 10)) 0x00004040`
	echo -n " $M" >> "$out"
done
echo >> "$out"

echo -n new_map >> "$out"
for pp in `seq 1 1 10`; do
	M=`do_bench $(((max_entries * pp) / 10)) 0x00006040`
	echo -n " $M" >> "$out"
done
echo >> "$out"

`readlink -f ../plot-it/plot-it.py` --xlabel "#fullness(%), max_entries=$max_entries, key_size=$key_size" --ylabel "M ops / second" $out
