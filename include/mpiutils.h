/**
 * @file mpiutils.c
 * @ingroup headers
 * @author Simone Bisogno (bissim.github.io)
 * @brief Header file for MPI utility functions.
 * @version 0.1.0-beta+20200316
 * @date 2020-02-07
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef MPIUTILS_H_
#define MPIUTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Default master (processor 0) in MPI cluster.
 * 
 * Default master (processor 0) in MPI cluster.
 */
static int MASTER = 0;
/**
 * @brief Default tag for MPI 1-to-1 communications.
 * 
 * Default tag for MPI 1-to-1 communications.
 */
static int TAG = 1;

void MPI_Printf(int, char *);
void MPI_Free(int, void *);
void checkMPIerror(int *, int *);

#ifdef __cplusplus
}
#endif

#endif // MPIUTILS_H_
