#!/bin/bash

duration=1;

if [ $# -eq 0 ]
then
    echo "// pass any parameter to the script to skip compilation.."
    ./scripts/create_glstm.sh;
fi;

if [ $? -eq 1 ];
then
    echo "!! ERROR: could not create the necessary executables for benchmarking";
    exit 1;
fi;

nc=$(nproc);

b0="bank_glstm"
b1="bank"

l0="ll_glstm"
l1="ll"


workloads="-r100 -r20 -r0";

rt=0;
ri=0;

echo "## Bank ##################################";
for w in $workloads;
do
    echo "# workload: $w";
    echo "#Thrd Throughput-gl Throughput-yours Ratio"
    for ((i = 1; i <= $nc; i++))
    do
	ri=$(($ri+1));
	printf "%-5d " $i;
	thr0=$(./$b0 $w -n$i -d$duration | awk '/# Commits/ { print $5 }');
	printf "%-13d " $thr0;
	thr1=$(./$b1 $w -n$i -d$duration | awk '/# Commits/ { print $5 }');
	printf "%-16d " $thr1;
	ratio=$(echo $thr1/$thr0 | bc -l);
	printf "%-7.2f\n" $ratio;
	rt=$(echo "$rt+$ratio" | bc -l);
    done;
done;

workloads="-u0 -u20 -u100";

echo "## LL ####################################";
for w in $workloads;
do
    echo "# workload: $w";
    echo "#Thrd Throughput-gl Throughput-yours Ratio"
    for ((i = 1; i <= $nc; i++))
    do
	ri=$(($ri+1));
	printf "%-5d " $i;
	thr0=$(./$l0 $w -n$i -d$duration | awk '/# Commits/ { print $5 }');
	printf "%-13d " $thr0;
	thr1=$(./$l1 $w -n$i -d$duration | awk '/# Commits/ { print $5 }');
	printf "%-16d " $thr1;
	ratio=$(echo $thr1/$thr0 | bc -l);
	printf "%-7.2f\n" $ratio;
	rt=$(echo "$rt+$ratio" | bc -l);
    done;
done;

bonus=$(echo "($rt/$ri) - 1" | bc -l);
bl=$(echo "$bonus > 1" | bc -l);
if [ $bl -eq 1 ];
then
    bonus=1;
fi;
printf "~~> Bonus points = ~%.1f \n" $bonus;
echo "(We will ofc run your code on a larger 40-core processor)";
echo "(Make certain that it scales with the number of threads)";
