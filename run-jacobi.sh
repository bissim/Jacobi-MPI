#!/bin/bash

#
# check for input parameters
#
TIME=$(date "+%Y.%m.%d-%H.%M.%S")
VERSION=$(cat ./VERSION)
ITERATIONS=6
SUCCESS=0
while [[ "$1" =~ ^- && ! "$1" == "--" ]]; do
    case $1 in
        ( -v | --version )
            echo $VERSION
            exit
            ;;
        ( -s | --serial )
            if [[ -n $TYPE ]]; then
                echo "Cannot specify both -s and -p parameter!"
                exit
            fi
            TYPE="serial"
            SUCCESS=1
            ;;
        ( -p | --parallel)
            if [[ -n $TYPE ]]; then
                echo "Cannot specify both -s and -p parameter!"
                exit
            fi
            TYPE="parallel"
            SUCCESS=1
            ;;
        ( -d | --dimension )
            shift;
            DIMENSION=$1
            if [[ ! $DIMENSION ]]; then
                echo "No dimension specified for -d option!"
                exit
            fi
            #echo "$DIMENSION specified as initial matrix dimension"
            SUCCESS=1
            ;;
        ( -D | --debug )
            DEBUG=1
            #echo "Debug option enabled"
            SUCCESS=1
            ;;
        ( -h | --help)
            echo "Jacobi MPI $VERSION"
            echo
            echo "Runs relaxed Jacobi over a random matrix of specified dimension"
            echo "for $ITERATIONS times, for each of them dimension gets doubled."
            echo "Elapsed times for each iterations are annotated within a log"
            echo "file."
            echo
            echo "Usage: $0 (--serial or --parallel) --dimension <dimension> --debug"
            echo
            echo "--serial or -s: run sequential Jacobi"
            echo "--parallel or -p: run parallel Jacobi"
            echo "--dimension or -d: matrix dimension at first execution"
            echo "--debug or -D: print debug information during execution"
            echo "--version or -v: print Jacobi MPI version"
            echo "--help or -h: print this program guide"
            exit
            ;;
        ( * )
            echo "Invalid parameter specified!"
            exit
            ;;
    esac;
    shift;
done
if [[ "$1" == '--' ]]; then shift; fi
if (( $SUCCESS == 0 )); then
    echo "No parameter specified!"
    exit
fi
if [[ ! $DIMENSION ]]; then
    echo "-d parameter is mandatory!"
    exit
fi

BINARY=jacobi-$TYPE
echo "Running $BINARY $ITERATIONS times over a $DIMENSION x $DIMENSION initial matrix"
if [[ $DEBUG ]]; then
    echo "Debug option enabled"
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
RESULT=""
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
    let DIMENSION=$DIMENSION*2
    let ITERATIONS=$ITERATIONS-1
    echo "-----" >> $OUTPUT
    echo "" >> $OUTPUT
done

echo
echo "End of tests for $BINARY"
