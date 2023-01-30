map_sizes="1000 4000 10000 20000 30000 40000 50000 60000 100000 200000 400000 600000 800000 1000000"

fullnesseseess="25 50 75 100"

for map_size in $map_sizes; do
	for fullness in $fullnesseseess; do
		echo "test map_size=$map_size fullness=$fullness"
		./run.bench-key-size.sh $map_size $fullness
	done
done
