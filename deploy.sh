#!/bin/bash

# strict mode
set -euo pipefail
IFS=$'\n\t'

SCRIPT_NAME=$0
JACOBI_MPI_VERSION=$(curl -s -L https://raw.githubusercontent.com/bissim/Jacobi-MPI/master/VERSION)
JACOBI_MPI_REPOSITORY=https://github.com/bissim/Jacobi-MPI
JACOBI_MPI_BIN=jacobi-mpi.tar.gz
JACOBI_MPI_BIN_URL=${JACOBI_MPI_REPOSITORY}/releases/download/${JACOBI_MPI_VERSION}/${JACOBI_MPI_BIN}
OPENMPI_VERSION=4.0.3
OPENMPI_ARCHIVE=openmpi-${OPENMPI_VERSION}.tar.gz
OPENMPI_RPM_NAME=openmpi-${OPENMPI_VERSION}-1.src
OPENMPI_RPM=${OPENMPI_RPM_NAME}.rpm
OPENMPI_URL=https://download.open-mpi.org/release/open-mpi/v4.0/${OPENMPI_ARCHIVE}
OPENMPI_RPM_URL=https://download.open-mpi.org/release/open-mpi/v4.0/${OPENMPI_RPM}
IS_MASTER=0
CLUSTER_DIM=0
HOSTFILE="hostfile"
PEM_KEY=""
ROOT=""
USERNAME=""
PASSWORD=""
PROGRAM=""
PROGRAM_DIM=""
RESULTS_FILE=""

#
# evaluate script parameters
#
while [[ "${1-0}" =~ ^- && ! "${1-0}" == "--" ]]; do
    case $1 in
        ( -k | --key )
            shift;
            PEM_KEY=$1
            ;;
        ( -R | --root )
            shift;
            ROOT=$1
            ;;
        ( -u | --username)
            shift;
            USERNAME=$1
            ;;
        ( -p | --password)
            shift;
            PASSWORD=$1
            ;;
        ( -n | --number-of-nodes )
            shift;
            CLUSTER_DIM=$1
            ;;
        ( -M | --master )
            IS_MASTER=1
            ;;
        ( -P | --program)
            shift;
            PROGRAM=$1
            ;;
        ( -r | --results-file )
            shift;
            RESULTS_FILE=$1
            ;;
        ( -d | --dimension )
            shift;
            PROGRAM_DIM=$1
            ;;
        ( * )
            echo "Invalid parameter specified!"
            exit
            ;;
    esac;
    shift;
done

#
# update remote repositories
#
echo -e "\nUpdate repositories"
sudo add-apt-repository universe
sudo apt update #&& sudo apt upgrade -y

#
# install build essential to build OpenMPI
# install alien for later use by everyone
#
echo -e "\nInstall dependencies"
sudo apt install build-essential alien -y

#
# download and extract OpenMPI source code
#
wget "$OPENMPI_URL" -O ./$OPENMPI_ARCHIVE && tar -xzf ./$OPENMPI_ARCHIVE
#wget "$OPENMPI_RPM_URL" -O ./$OPENMPI_RPM

#
# configure, build and install OpenMPI
# note: this won't work on t2.micro instances!
#
# compile from binaries
cd ./openmpi-${OPENMPI_VERSION}/
./configure --prefix="/usr/local"
make -j && sudo make install && sudo ldconfig
cd ..
# install from APT
#sudo apt install openmpi-bin libopenmpi-dev -y
# install from RPT package
#sudo alien ${OPENMPI_RPM}
#sudo dpkg -i ${OPENMPI_RPM_NAME}.deb

#
# download and extract binaries
#
#echo -e "\nDownload $JACOBI_MPI_BIN archive"
#wget "${JACOBI_MPI_BIN_URL}" -O ./$JACOBI_MPI_BIN && tar -xzf ./$JACOBI_MPI_BIN
#sudo cp ./bin/$PROGRAM /home/$USERNAME/$PROGRAM

#
# MASTER has more things to do then
#
if (( $IS_MASTER == 1 )); then
    #
    # install LLVM repository
    #
    LLVM_APT_SOURCE=llvm.list
    if [[ ! -e /etc/apt/sources.list.d/$LLVM_APT_SOURCE ]]; then
        echo -e "\nSet LLVM repositories"
        touch $LLVM_APT_SOURCE
        echo "# i386 not available" > $LLVM_APT_SOURCE
        echo "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic main" >> $LLVM_APT_SOURCE
        echo "deb-src http://apt.llvm.org/bionic/ llvm-toolchain-bionic main" >> $LLVM_APT_SOURCE
        echo "# 9" >> $LLVM_APT_SOURCE
        echo "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-9 main" >> $LLVM_APT_SOURCE
        echo "deb-src http://apt.llvm.org/bionic/ llvm-toolchain-bionic-9 main" >> $LLVM_APT_SOURCE
        echo "# 10" >> $LLVM_APT_SOURCE
        echo "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-10 main" >> $LLVM_APT_SOURCE
        echo "deb-src http://apt.llvm.org/bionic/ llvm-toolchain-bionic-10 main" >> $LLVM_APT_SOURCE
        sudo mv $LLVM_APT_SOURCE /etc/apt/sources.list.d/
        sudo chmod 644 /etc/apt/sources.list.d/$LLVM_APT_SOURCE
        sudo chown root:root /etc/apt/sources.list.d/$LLVM_APT_SOURCE
        wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
    fi

    #
    # install Clang
    #
    echo -e "\nInstall Clang"
    sudo apt update && sudo apt install clang -y

    #
    # clone and compile project
    #
    if [[ -e ./Jacobi-MPI/ ]]; then
        rm -r ./Jacobi-MPI/
    fi
    git clone "$JACOBI_MPI_REPOSITORY.git"
    cd Jacobi-MPI/
    echo -e "\nBuilding Jacobi MPI, version $(cat ./VERSION)"
    mkdir bin
    make
    cd ..
    mv ./Jacobi-MPI/bin/$PROGRAM .

    #
    # download and extract binaries
    #
    #echo -e "\nDownload $JACOBI_MPI_BIN archive"
    #wget "${JACOBI_MPI_BIN_URL}" -O ./$JACOBI_MPI_BIN && tar -xzf ./$JACOBI_MPI_BIN
    sudo cp ./$PROGRAM /home/$USERNAME/$PROGRAM
    sudo chown $USERNAME:root /home/$USERNAME/$PROGRAM
    sudo chmod 777 /home/$USERNAME/$PROGRAM

    #
    # distribute parallel binaries to all nodes
    #
    echo -e "\nDistributing $PROGRAM to all cluster nodes..."
    for (( i=1; i<$CLUSTER_DIM; i++ )); do
        sudo -u $USERNAME scp -oStrictHostKeyChecking=no /home/$USERNAME/$PROGRAM $USERNAME@NODE_${i}:~
    done

    #
    # generate the hostfile
    #
    echo -e "\nGenerating hostfile for $CLUSTER_DIM nodes..."
    #if [[ -e $HOSTFILE ]]; then
    #    > $HOSTFILE
    #fi
    #echo "MASTER" > $HOSTFILE
    #for (( i=1; i<$CLUSTER_DIM; i++ )); do
    #    echo "NODE_$i" >> $HOSTFILE # these won't do
    #done
    #sudo mv $HOSTFILE /home/$USERNAME/$HOSTFILE
    . ip_private_list.array
    #for (( i=1; i<=${#ip_private_list[@]}; i++ ))
    #do
    #    ## erase current hostfile if already exists
    #    if [[ -e $HOSTFILE"_$i" ]]
    #    then
    #        > $HOSTFILE"_$i"
    #    fi
    #    for private_ip in "${ip_private_list[@]:0:$i}"
    #    do
    #        echo $private_ip >> $HOSTFILE"_$i"
    #	 done
    #    sudo mv $HOSTFILE"_$i" /home/$USERNAME/$HOSTFILE"_$i"
    #done
    ## erase current hostfile if already exists
    if [[ -e $HOSTFILE ]]; then
        > $HOSTFILE
    fi
    for private_ip in "${#ip_private_list[@]}"; do
        echo $private_ip >> $HOSTFILE
    done
    sudo mv $HOSTFILE /home/$USERNAME/$HOSTFILE

    #
    # copy user keys to send and run scripts remotely
    #
    echo -e "\nCopying $USERNAME key to communicate with oher nodes..."
    sudo cp /home/pcpc/.ssh/id_rsa .ssh/
    sudo cp /home/pcpc/.ssh/id_rsa.pub .ssh/
    sudo chown $ROOT:root .ssh/id_rsa .ssh/id_rsa.pub

    #
    # propagate this script to every slave node
    #
    echo -e "\nPropagating $SCRIPT_NAME to other nodes..."
    for (( i=1; i<$CLUSTER_DIM; i++ )); do
        # first, send the script
        echo -e "\nCopying $SCRIPT_NAME on NODE_$i..."
        sudo cp ./$SCRIPT_NAME /home/$USERNAME/
        sudo -u $USERNAME scp -oStrictHostKeyChecking=no /home/$USERNAME/$SCRIPT_NAME $USERNAME@NODE_${i}:~

        # then, execute it
        echo -e "\nRunning $SCRIPT_NAME on NODE_$i..."
        REMOTE_COMMAND="/home/$USERNAME/${SCRIPT_NAME} -R $ROOT -u $USERNAME -p $PASSWORD"
        ssh -oStrictHostKeyChecking=no -i $PEM_KEY $ROOT@NODE_${i} "$REMOTE_COMMAND" &
    done

    #
    # wait for nodes preparation running in background
    #
    wait
    echo -e "\nCluster is ready to run $PROGRAM in parallel with MPI!"

    #
    # at last, run MPI program (actually, not here!)
    #
    #echo -e "\nRunning $PROGRAM..."
    #NPROC=$(nproc)*CLUSTER_DIM
    #sudo -u $USERNAME mpiexec -np $NPROC --hostfile /home/$USERNAME/$HOSTFILE"_$CLUSTER_DIM" /home/$USERNAME/$PROGRAM $PROGRAM_DIM /home/$USERNAME/$RESULTS_FILE
fi
