#! /bin/bash -x

key_size="${1:-4}"
map_flags="${2:-0}"
nr_entries=65536
max_entries=65536

bench=${bench:-`readlink -f ~/src/bpf-next/tools/testing/selftests/bpf/bench`}

sudo $bench -d2 -a bpf-hashmap-lookup \
	--key_size=$key_size \
	--nr_entries=$nr_entries \
	--max_entries=$max_entries \
	--map_flags=$map_flags \
	#
