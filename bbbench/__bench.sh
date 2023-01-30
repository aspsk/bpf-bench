#! /bin/bash -efu

dir=$(readlink -f $(pwd))

hash_func="$1"
start="$2"
step="$3"
end="$4"
output_filename="$dir/$5"

rmmod "bbbench" || :
insmod "$dir/bbbench.ko"

cd "/sys/kernel/bbbench"

touch "$output_filename"

echo "$hash_func" > run
echo -n "$hash_func" >>"$output_filename"
for i in $(seq $start $step $end); do
	echo $i > run
	eval `cat run`
	echo -n " $mean" >> "$output_filename"
done
echo >> "$output_filename"
