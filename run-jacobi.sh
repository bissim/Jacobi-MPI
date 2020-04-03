#!/bin/bash

# strict mode
set -euo pipefail
IFS=$'\n\t'

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
# check for input parameters
#
TIME=$(date "+%Y.%m.%d-%H:%M:%S")
VERSION=$(cat ./VERSION)
NPROC_MAX=$(nproc)
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
echo -e "\nRunning $BINARY $ITERATIONS times over a"
echo -e "\r $DIMENSION x $DIMENSION initial matrix"
if (( $DEBUG == 1 )); then
    echo "Debug option enabled"
fi
echo

#
# run chosen executable ITERATIONS times
#
RESULTFILE="results-$TYPE" # TODO fix name generation
if [[ ! -e ./log ]]; then
    echo "Creating log/ directory..."
    mkdir log
else
    echo "log/ directory already exists"
fi
OUTPUT=./log/$BINARY.log
echo "Output will be saved in $OUTPUT"
echo
echo -e "\n\t#####\n" >> $OUTPUT
echo "[$TIME] Running Jacobi MPI $VERSION, $TYPE esecution" >> $OUTPUT

if [[ $TYPE == "serial" ]]; then
    #
    # compile dependencies and chosen executable
    # if binary file doesn't already exist
    #
    if [[ ! -e ./bin/$BINARY ]]; then
        echo "Building $BINARY..." | tee -a $OUTPUT
        if [[ ! -e ./bin ]]; then
            echo "Creating bin/ directory..." | tee -a $OUTPUT
            mkdir bin
        else
            echo "bin/ directory already exists" | tee -a $OUTPUT
        fi
        make jacobiutils
        make $BINARY
    else
        echo "$BINARY has already been built" | tee -a $OUTPUT
    fi
    echo | tee -a $OUTPUT

    #
    # run jacobi-serial
    #
    RESULTFILE="${RESULTFILE}.csv"
    echo "\"Size\",\"Time\",\"TimeMin\",\"TimeMax\"" > ./data/$RESULTFILE
    for (( I = 1; I <= $ITERATIONS; I++ )); do
        echo "$DIMENSION x $DIMENSION matrix"
        echo -e "\tBEGIN ITERATION $I FOR $DIMENSION x $DIMENSION MATRIX\n" >> $OUTPUT
        for (( J = 0; J < $MEASUREITERATIONS; J++ )); do
            # stuff
            echo -e "\n\tEXECUTION $J\n" >> $OUTPUT
            ./bin/$BINARY $DIMENSION ./data/$RESULTFILE $DEBUG >> $OUTPUT
        done
        reduce ./data/$RESULTFILE $DIMENSION
        let DIMENSION=$DIMENSION*2
        echo -e "\n\t-----\n" >> $OUTPUT
    done
elif [[ $TYPE == "parallel" ]]; then
    WEAK_EXT="-w.csv"
    STRONG_EXT="-s.csv"

    # consider doing a local test for strong and weak scaling
    if (( $NPROC_MAX >= 8 )); then
        echo -e "\nPerforming local scaling tests..." | tee -a $OUTPUT
        ORIGINAL_DIM=$DIMENSION
        HEADER="\"No. of processors\",\"Time\",\"TimeMin\",\"TimeMax\""

        # strong scaling local test
        echo -e "\nLocal strong scaling test..." | tee -a $OUTPUT
        echo "Starting dimension: $DIMENSION" | tee -a $OUTPUT
        # increase dimension to maxiumu value first
        for (( $NPROC = 2; $NPROC <= $NPROC_MAX; $NPROC = $NPROC * 2 )); do
            DIMENSION=${DIMENSION}*2
        done
        MAX_DIM=$DIMENSION

        echo $HEADER > ./data/$RESULTFILE"-l$STRONG_EXT"
        for (( $NPROC = 2; $NPROC <= $NPROC_MAX; $NPROC = $NPROC * 2 )); do
            for (( J = 0; J < $MEASUREITERATIONS; J++ )); do
                mpiexec -np $NPROC --use-hwthread-cpus ./bin/$BINARY $DIMENSION $RESULTFILE"-l$STRONG_EXT" $DEBUG >> $OUTPUT
            done
            reduce ./data/$RESULTFILE"-l$STRONG_EXT" $NPROC
        done

        # weak scaling  local test
        echo -e "\nLocal weak scaling test..." | tee -a $OUTPUT
        echo "Starting dimension: $DIMENSION" | tee -a $OUTPUT
        DIMENSION=$ORIGINAL_DIM
        echo $HEADER > ./data/$RESULTFILE"-l$WEAK_EXT"
        for (( $NPROC = 2; $NPROC <= $NPROC_MAX; $NPROC = $NPROC * 2 )); do
            for (( J = 0; J < $MEASUREITERATIONS; J++ )); do
                mpiexec -np $NPROC --use-hwthread-cpus ./bin/$BINARY $DIMENSION $RESULTFILE"-l$WEAK_EXT" $DEBUG >> $OUTPUT
            done
            reduce ./data/$RESULTFILE"-l$WEAK_EXT" $NPROC
            DIMENSION=${DIMENSION}*2
        done
        DIMENSION=$ORIGINAL_DIM

        # pause for debug purposes
        echo -e "Press ENTER to continue..."
        read KB_INPUT
    else
        echo -e "\nNot enough cores ($NPROC_MAX) to perform local tests." | tee -a $OUTPUT
    fi

    CLUSTER_SIZE=8
    echo -e "\nRunning in parallel over a cluster of $CLUSTER_SIZE instances" | tee -a $OUTPUT

    # copy AWS Build Cluster Script in some subfolder
    if [[ -e ./aws-build-cluster-script/ ]]; then
        rm -r ./aws-build-cluster-script/
    fi
    git clone https://github.com/bissim/aws-build-cluster-script.git
    rm -rf ./aws-build-cluster-script/.git/
    if [[ -e ./scripts/ ]]; then
        rm -rf ./scripts/
    fi
    mv ./aws-build-cluster-script ./scripts

    # copy existing EC2 key
    KEY="pcpc-key"
    PEM_KEY="${KEY}.pem"
    read -p "Path to EC2 key: " KEY_PATH
    cp $KEY_PATH ./scripts/key/$PEM_KEY
    chmod 400 ./scripts/key/*.pem

    # create cluster
    EC2_AMI="ami-07ebfd5b3428b6f4d"
    EC2_SG="sg-037781947b5515887"
    EC2_TYPE="m4.xlarge" # m4.xlarge
    ROOT="ubuntu"
    USERNAME="pcpc"
    PASSWORD="pcpc-test"
    echo -e "\nCreating a $CLUSTER_SIZE nodes cluster with following features:" | tee -a $OUTPUT
    echo -e "AMI:\t$EC2_AMI" | tee -a $OUTPUT
    echo -e "Security group:\t$EC2_SG" | tee -a $OUTPUT
    echo -e "Instance type:\t$EC2_TYPE" | tee -a $OUTPUT
    echo -e "Key:\t$KEY" | tee -a $OUTPUT
    echo -e "Instance root user:\t$ROOT" | tee -a $OUTPUT
    echo -e "Custom user name:\t$USERNAME" | tee -a $OUTPUT
    echo -e "Custom user password:\t$PASSWORD" | tee -a $OUTPUT
    echo -e "\nBe sure that $PEM_KEY is in ./scripts/key directory!" | tee -a $OUTPUT
    cd ./scripts
    chmod +x ./make_cluster.sh ./state_cluster.sh
    ./make_cluster.sh $EC2_AMI $ROOT $EC2_SG $EC2_TYPE $KEY $CLUSTER_SIZE $USERNAME $PASSWORD
    cd ..

    # send preparation script to master and run it
    #. ./scripts/data/ip_private_list.array
    . ./scripts/data/ip_list.array
    MASTER_IP=${ip_list[0]}
    echo "Master IP is $MASTER_IP" | tee -a $OUTPUT
    DEPLOY_SCRIPT=deploy.sh
    chmod +x ./$DEPLOY_SCRIPT
    echo -e "\nDeploying $DEPLOY_SCRIPT over MASTER..." | tee -a $OUTPUT
    scp -i ./scripts/key/$PEM_KEY ./$DEPLOY_SCRIPT $ROOT@$MASTER_IP:~
    REMOTE_COMMAND="chmod +x $DEPLOY_SCRIPT &&"
    REMOTE_COMMAND="${REMOTE_COMMAND} ./$DEPLOY_SCRIPT -k $PEM_KEY -R $ROOT"
    REMOTE_COMMAND="${REMOTE_COMMAND} -u $USERNAME -p $PASSWORD -n $CLUSTER_SIZE -M"
    REMOTE_COMMAND="${REMOTE_COMMAND} -P $BINARY -d $DIMENSION"

    # install dependencies over cluster
    echo -e "\nExecuting $DEPLOY_SCRIPT on MASTER..." | tee -a $OUTPUT
    ssh -i ./scripts/key/$PEM_KEY $ROOT@$MASTER_IP $REMOTE_COMMAND

    # set results files header
    echo -e "\nSetting results file headers..." | tee -a $OUTPUT
    HEADER="\"No. of processors\",\"Time\",\"TimeMin\",\"TimeMax\""
    echo "Sending header files to MASTER..." | tee -a $OUTPUT
    REMOTE_COMMAND="echo \"${HEADER}\" > ./${RESULTFILE}${STRONG_EXT}"
    ssh -i ./scripts/key/$PEM_KEY $ROOT@$MASTER_IP $REMOTE_COMMAND
    REMOTE_COMMAND="echo \"${HEADER}\" > ./${RESULTFILE}${WEAK_EXT}"
    ssh -i ./scripts/key/$PEM_KEY $ROOT@$MASTER_IP $REMOTE_COMMAND

    # run parallel program for strong scaling
    # fixed dimension, doubling number of processes
    echo -e "\nRunning strong scaling test for $BINARY..." | tee -a $OUTPUT
    echo "Starting dimension: $DIMENSION" | tee -a $OUTPUT
    NPROC=2
    for (( I = 1; I <= $ITERATIONS; I++ )); do
        echo "Iteration $I of $BINARY, strong scaling" | tee -a $OUTPUT
        REMOTE_COMMAND="sudo -u $USERNAME mpiexec -np $NPROC"
        REMOTE_COMMAND="${REMOTE_COMMAND} --hostfile /home/$USERNAME/$HOSTFILE"
        REMOTE_COMMAND="${REMOTE_COMMAND} /home/$USERNAME/$PROGRAM $PROGRAM_DIM"
        REMOTE_COMMAND="${REMOTE_COMMAND} /home/$USERNAME/${RESULTFILE}${STRONG_EXT}"
        # run the same command few times to get estimation
        for (( J = 0; J < $MEASUREITERATIONS; J++ )); do
            ssh -i ./scripts/key/$PEM_KEY $ROOT@$MASTER_IP $REMOTE_COMMAND
        done

        # reduce results at every iteration
        # download, reduce, upload
        # stupid but quick and effective right now
        echo "Reducing results file for strong scaling..." | tee -a $OUTPUT
        scp -i ./scripts/key/$PEM_KEY $ROOT@$MASTER_IP:$RESULTFILE"$STRONG_EXT" ./data/$RESULTFILE"$STRONG_EXT"
        reduce ./data/$RESULTFILE"$STRONG_EXT" $NPROC
        scp -i ./scripts/key/$PEM_KEY ./data/$RESULTFILE"$STRONG_EXT" $ROOT@$MASTER_IP:$RESULTFILE"$STRONG_EXT"

        # it's strong scaling, so doubling nproc
        NPROC=${NPROC}*2
    done
    # retrieve strong scaling tests results
    scp -i ./scripts/key/$PEM_KEY $ROOT@$MASTER_IP:$RESULTFILE"$STRONG_EXT" ./data/$RESULTFILE"$STRONG_EXT"

    # run parallel program for weak scaling
    # doubling both dimension and number of processes
    echo -e "\nRunning weak scaling test for $BINARY..." | tee -a $OUTPUT
    echo "Starting dimension: $DIMENSION" | tee -a $OUTPUT
    NPROC=2
    for (( I = 1; I <= $ITERATIONS; I++ )); do
        echo "Iteration $I of $BINARY, weak scaling"
        REMOTE_COMMAND="sudo -u $USERNAME mpiexec -np $NPROC"
        REMOTE_COMMAND="${REMOTE_COMMAND} --hostfile /home/$USERNAME/$HOSTFILE"
        REMOTE_COMMAND="${REMOTE_COMMAND} /home/$USERNAME/$PROGRAM $PROGRAM_DIM"
        REMOTE_COMMAND="${REMOTE_COMMAND} /home/$USERNAME/${RESULTFILE}${WEAK_EXT}"
        # run the same command few times to get estimation
        for (( J = 0; J < $MEASUREITERATIONS; J++ )); do
            ssh -i ./scripts/key/$PEM_KEY $ROOT@$MASTER_IP $REMOTE_COMMAND
        done

        # reduce results at every iteration
        # download, reduce, upload
        # stupid but quick and effective right now
        echo "Reducing results for weak scaling..." | tee -a $OUTPUT
        scp -i ./scripts/key/$PEM_KEY $ROOT@$MASTER_IP:$RESULTFILE"$WEAK_EXT" ./data/$RESULTFILE"$WEAK_EXT"
        reduce ./data/$RESULTFILE"$WEAK_EXT" $NPROC
        scp -i ./scripts/key/$PEM_KEY ./data/$RESULTFILE"$WEAK_EXT" $ROOT@$MASTER_IP:$RESULTFILE"$WEAK_EXT"

        # it's weak scaling, so doubling both nproc and input dimension
        NPROC=${NPROC}*2
        DIMENSION=${DIMENSION}*2
    done
    # retrieve weak scaling tests results
    scp -i ./scripts/key/$PEM_KEY $ROOT@$MASTER_IP:$RESULTFILE"$WEAK_EXT" ./data/$RESULTFILE"$WEAK_EXT"

    # terminate cluster
    echo -e "\nTerminating cluster (a prompt may appear, press 'q')..." | tee -a $OUTPUT
    ./scripts/state_cluster stop # TODO replace with 'terminate'
fi
echo -e "\n\t#####" >> $OUTPUT

#
# print or plot results
#
if (( $PRINT == 1 )); then
    # print results
    echo
    echo -e "\nResults of $BINARY execution"
    if [[ $TYPE == "serial" ]]; then
        echo "Local $TYPE results:"
        printresults ./data/$RESULTFILE
    elif [[ $TYPE == "parallel" ]]; then
        if (( $NPROC_MAX >= 8 )); then
            echo "Local $TYPE results:"
            printresults ./data/$RESULTFILE"-l$STRONG_EXT"
            printresults ./data/$RESULTFILE"-l$WEAK_EXT"
        fi
        echo "Remote $TYPE results:"
        printresults ./data/$RESULTFILE"$STRONG_EXT"
        printresults ./data/$RESULTFILE"$WEAK_EXT"
    fi
else
    # plot execution times
    RESULTPLOT="./src/results.plt"
    GPTLOG=./log/gpt-$BINARY.log
    echo $TIME >> $GPTLOG
    echo
    echo -e "\nGenerating execution time graphs..." | tee -a $OUTPUT
    if [[ $TYPE == "serial" ]]; then
        gnuplot -c $RESULTPLOT ./data/$RESULTFILE $TYPE 2>> $GPTLOG
        echo -e "\n-----" >> $GPTLOG
        echo "Plot saved!" | tee -a $OUTPUT
    elif [[ $TYPE == "parallel" ]]; then
        if (( $NPROC_MAX >= 8 )); then
            # process local scaling tests results
            gnuplot -c $RESULTPLOT ./data/$RESULTFILE"-l$WEAK_EXT" $TYPE" weak scaling" 2>> $GPTLOG
            echo "-----" >> $GPTLOG
            gnuplot -c $RESULTPLOT ./data/$RESULTFILE"-l$STRONG_EXT" $TYPE" strong scaling" 2>> $GPTLOG
            echo -e "\n-----" >> $GPTLOG
        fi

        gnuplot -c $RESULTPLOT ./data/$RESULTFILE"$WEAK_EXT" $TYPE" weak scaling" 2>> $GPTLOG
        echo "-----" >> $GPTLOG
        gnuplot -c $RESULTPLOT ./data/$RESULTFILE"$STRONG_EXT" $TYPE" strong scaling" 2>> $GPTLOG
        echo -e "\n-----" >> $GPTLOG
        echo "Plots saved!" | tee -a $OUTPUT
    fi
fi

echo
echo -e "\nEnd of tests for $BINARY" | tee -a $OUTPUT
