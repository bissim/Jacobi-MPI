#!/bin/bash

#
# check for input parameters
#
if (( $# < 2 )); then
    if (( $# == 0 )); then
        echo "No parameter specified!"
    else
        echo "Only $# parameters specified!"
    fi
    echo "Use of script: $0 <serial|parallel> <numberOfValues> <printFlag>"
    exit
fi

#
# determine executable to run
#
TYPE=$1
TYPE=${TYPE,,}
if [[ $TYPE == "serial" ]] || [[ $TYPE == "parallel" ]]; then
    echo "[`date "+%Y.%m.%d-%H.%M.%S"`] Running $TYPE version of relaxed Jacobi" | tee $OUTPUT
    BINARY=jacobi-$TYPE
else
    echo "Unknown execution type: $TYPE"
    echo "Please specify 'serial' or 'parallel'"
    exit
fi

#
# compile dependencies and chosen executable
#
echo "Building $BINARY..."
echo
make jacobiutils
make $BINARY

#
# run chosen executable ITERATIONS times
#
DIMENSION=$2
DEBUG=${3:-0}
RESULT=""
ITERATIONS=6
OUTPUT=$BINARY.log
NPROC=`nproc`
echo " Output will be saved in $OUTPUT"
#echo "[`date "+%Y.%m.%d-%H.%M.%S"`] $TYPE esecution" > $OUTPUT
while (( $ITERATIONS >= 0 )); do
    echo "$DIMENSION x $DIMENSION matrix"
    if [[ $TYPE == "serial" ]]; then
        ./bin/$BINARY $DIMENSION $DEBUG >> $OUTPUT
    elif [[ $TYPE == "parallel" ]]; then
        echo "Running in parallel over $NPROC processors"
        mpiexec -np $NPROC --use-hwthread-cpus ./bin/$BINARY $DIMENSION $DEBUG >> $OUTPUT
    fi
    DIMENSION=$DIMENSION*2
    ITERATIONS=$ITERATIONS-1
    echo "-----" >> $OUTPUT
    echo "" >> $OUTPUT
done

echo
echo "End of tests for $BINARY"
