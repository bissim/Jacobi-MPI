/**
 * @file mpiutils.c
 * @ingroup libraries
 * @author Simone Bisogno (bissim.github.io)
 * @brief MPI utility functions.
 * @version 0.1.0-rc.3+20200406
 * @date 2020-02-07
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <stdio.h>
#include <stdlib.h>

#include "mpi.h"
#include "mpiutils.h"

/**
 * @brief Default master (processor 0) in MPI cluster.
 * 
 * Default master (processor 0) in MPI cluster.
 */
extern int MASTER;
/**
 * @brief Default tag for MPI 1-to-1 communications.
 * 
 * Default tag for MPI 1-to-1 communications.
 */
extern int TAG;

// void MPI_Pause(int process, int master, MPI_Comm comm) {
//     if (process == master) {
//         printf("[P%d] Press ENTER to continue...\n", process);
//         fflush(stdout);
//         getchar();
//     }
//     else {
//         printf("[P%d] Waiting for MASTER...\n", process);
//         fflush(stdout);
//     }
//     MPI_Barrier(comm);
// }

/**
 * Funzione che stampa un messaggio con il
 * rank del processo come prefisso
 */
void MPI_Printf(int process, char *message) {
    printf("[P%d] %s", process, message);
}

/**
 * Funzione per liberare memoria
 * dal processo master
 */
void MPI_Free(int rank, void *memory) {
    if (rank == MASTER && memory != NULL) {
        free(memory);
        memory = NULL;
    }
    else if (rank == MASTER && memory == NULL) {
        MPI_Printf(rank, "Trying to free null memory!\n");
    }
}

/**
 * Funzione per controllare l'esito
 * di una chiamata a funzione MPI
 */
void checkMPIerror(int *process, int *error) {
    if (*error != MPI_SUCCESS) {
        fprintf(stderr, "[P%d] MPI call failed!\nError %d.\n", *process, *error);
        fflush(stderr);

        exit(EXIT_FAILURE);
    }
    // else {
    //     MPI_Printf(*process, "MPI call successful!\n");
    // }
}
