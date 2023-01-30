map_sizes="1000 4000 10000 20000 30000 40000 50000 60000 100000 200000 400000 600000 800000 1000000"

key_sizes="4 8 12 16 20 24 28 32 36 40 64 128 200"

for map_size in $map_sizes; do
	for key_size in $key_sizes; do
		echo "test map_size=$map_size key_size=$key_size"
		./run.bench-fullness.sh $map_size $key_size
	done
done
