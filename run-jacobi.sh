#!/bin/bash

# strict mode
set -euo pipefail
IFS=$'\n\t'

#
# check for input parameters
#
TIME=$(date "+%Y.%m.%d-%H:%M:%S")
VERSION=$(cat ./VERSION)
ITERATIONS=6
MEASUREITERATIONS=3
SUCCESS=0
TYPE=""
DEBUG=0
PRINT=0
while [[ "${1-0}" =~ ^- && ! "${1-0}" == "--" ]]; do
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
        ( -R | --printresults )
            PRINT=1
            #echo "Print results option enabled"
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
            echo "--printresults or -R: print results from execution on stdout instead of plotting"
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
if [[ "${1-0}" == '--' ]]; then shift; fi
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
if (( $DEBUG == 1 )); then
    echo "Debug option enabled"
fi
echo

#
# compile dependencies and chosen executable
# if binary file doesn't already exist
#
echo "Building $BINARY..."
if [[ ! -e ./bin/$BINARY ]]; then
    make jacobiutils
    make $BINARY
else
    echo "$BINARY has already been built!"
fi
echo

#
# define utility functions
#
reduce() {
    local filename=$1
    local dimension=$2

    # extract from CSV file the line with $dimension as first value
    # then cut away comma-separated columns from second one
    # at last, sort values from lower to upper
    local values=(`grep "^$dimension," $filename | cut -d "," -f 2 | sort`)

    # put lower value in 2nd place
    # and higher one in 3rd place
    local temp=${values[1]}
    values[1]=${values[0]}
    values[0]=$temp

    # create new file line
    local line=$dimension,${values[@]}
    #echo $line

    # replace lines with DIMENSION
    # and make them unique
    echo "$(sed "s/^$dimension,.*/$line/" $filename | uniq)" > $filename
    echo "$(tr " " "," < $filename)" > $filename
}

printresults() {
    local results=$1

    # print results to stdout
    echo "$(tr "," "\t" < $results)"
}

#
# run chosen executable ITERATIONS times
#
RESULTFILE="./data/results-$TYPE.csv"
OUTPUT=./log/$BINARY.log
NPROC=`nproc`
echo "Output will be saved in $OUTPUT"
#echo "[`date "+%Y.%m.%d-%H.%M.%S"`] $TYPE esecution" > $OUTPUT
echo "\"Size\",\"Time\",\"TimeMin\",\"TimeMax\"" > $RESULTFILE
for (( I = 0; I < $ITERATIONS; I++ )); do
    echo "$DIMENSION x $DIMENSION matrix"
    if [[ $TYPE == "serial" ]]; then
        for (( J = 0; J < $MEASUREITERATIONS; J++ )); do
            # stuff
            ./bin/$BINARY $DIMENSION $RESULTFILE $DEBUG >> $OUTPUT
        done
    elif [[ $TYPE == "parallel" ]]; then
        echo "Running in parallel over $NPROC processors"
        # TODO weak and strong scalability graph generation coming soon
        mpiexec -np $NPROC --use-hwthread-cpus ./bin/$BINARY $DIMENSION $DEBUG >> $OUTPUT
    fi
    reduce $RESULTFILE $DIMENSION
    let DIMENSION=$DIMENSION*2
    echo "-----" >> $OUTPUT
    echo "" >> $OUTPUT
done

#
# print or plot results
#
if (( $PRINT == 1 )); then
    # print results
    echo
    echo "$RESULTFILE"
    printresults $RESULTFILE
else
    # plot execution times
    RESULTPLOT="./src/results.plt"
    GPTLOG=./log/gpt-$BINARY.log
    echo $TIME >> $GPTLOG
    echo
    echo "Generating execution time graph..."
    gnuplot -c $RESULTPLOT $RESULTFILE $TYPE 2>> $GPTLOG
    echo "-----" >> $GPTLOG
    echo "Plot saved to $RESULTPLOT"

    echo
    echo "End of tests for $BINARY"
fi
