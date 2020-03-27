/**
 * @file jacobi-parallel.c
 * @ingroup runnable
 * @author Simone Bisogno (bissim.github.io)
 * @brief Parallel version of Jacobi method.
 * @version 0.1.0-beta+20200316
 * @date 2020-02-04
 *
 * @copyright Copyright (c) 2020
 *
 */
/**
 * @brief Use timespec definition from POSIX.
 * 
 * Use `timespec` definition from POSIX.
 */
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "mpi.h"
#include "matrixutils.h"
#include "jacobi.h"
#include "mpiutils.h"
#include "misc.h"

/**
 * @brief How many milliseconds in a second.
 * 
 * How many milliseconds in a second.
 */
extern const int MS_IN_S;
/**
 * @brief Minimum double precision floating point number to be generated.
 */
extern const double LOWER_BOUND;
/**
 * @brief Maximum double precision floating point number to be generated.
 */
extern const double UPPER_BOUND;

/**
 * @brief The main function of Jacobi method in parallel version.
 * 
 * @param argc Count of command-line parameters
 * @param argv Command-line parameters
 * @return int Return value indicating whether program execution succeded
 */
int main (int argc, char **argv) {
    // MPI management variables
    int nproc;
    int me;
    const MPI_Comm COMM = MPI_COMM_WORLD;
    MPI_Status status;
    int *sendcounts, *recvcounts;
    int *senddispls, *recvdispls;
    extern int MASTER; // TODO try nproc - 1;
    extern int TAG;

    // time management variables
    double t_start;
    double t_end;
    double t_max;

    // program execution management
    unsigned char debug = 0;
    char *output_file;
    FILE *results;

    // business variables
    /**
     * @brief The order of the matrix associated to linear system
     *
     */
    int n;
    int local_rows;
    int local_g_rows;
    int first_g_row;
    int last_g_row;
    int last_local_row;
    /**
     * @brief The coefficient matrix in linear system
     *
     */
    double *A;
    double *local_A;
    double *local_A_g;
    double *local_A_g_prime;
    int num_iterations;
    double diffnorm;
    double local_diffnorm;

    // initialize MPI environment
    MPI_Init(&argc, &argv);

    MPI_Comm_size(COMM, &nproc);
    MPI_Comm_rank(COMM, &me);

    if (me == MASTER) {
        printf("Running %s over %d processes...\n\n\v", argv[0], nproc);
        fflush(stdout);
    }
    if (debug && me != MASTER) {
        printf("Running %s over %d processes...\n\n\v", argv[0], nproc);
        fflush(stdout);
    }

    // check for command-line arguments
    if (argc < 3) {
        if (me == MASTER) {
            printf("\aInsufficient number of parameters!\n");
            printf("Usage: %s <matrixOrder> <outputFileName> [<debugFlag>]\n\n", argv[0]);
            fflush(stdout);
        }

        MPI_Abort(COMM, EXIT_FAILURE);
		exit(EXIT_FAILURE);
	}
    else if (argc == 3) {
        n = atoi(argv[1]);
        output_file = malloc ((strlen(argv[2]) + 1) * sizeof output_file);
        sprintf(output_file, "%s", argv[2]);
        debug = 0;
    }
	else {
		n = atoi(argv[1]);
        output_file = malloc ((strlen(argv[2]) + 1) * sizeof output_file);
        sprintf(output_file, "%s", argv[2]);
        debug = (unsigned char) atoi(argv[3]);
	}

    if (me == MASTER) {
        printf("Matrix dimension: %dx%d (%d elements)\n", n, n, n*n);
        printf("\n");
        fflush(stdout);
    }

    // check whether number of processor is power of 2
    if ((nproc & (nproc - 1)) != 0) {
        if (me == MASTER) {
            fprintf(
                stderr,
                "\a[P%d] Number of processor must be a power of 2!",
                me
            );
        }

        MPI_Abort(COMM, EXIT_FAILURE);
    }
    else if (n == nproc) {
        if (me == MASTER) {
            fprintf(
                stderr,
                "\a[P%d] Matrix size must be such that every processor receives ",
                me
            );
            fprintf(
                stderr,
                "at least 2 rows (%d/%d is 1)!",
                n,
                nproc
            );
        }

        MPI_Abort(COMM, EXIT_FAILURE);
    }

    if (debug) {
        printf("\n\t\t>>> PROCESS %d OF %d <<<\n\n", me, nproc);
        fflush(stdout);
    }

    // generate matrix vector
    A = malloc(n * n * sizeof *A);
    if (me == MASTER) {
        if (debug) {
            printf("[P%d] Generating matrix...\n", me);
            printf("\n");
            fflush(stdout);
        }

        generate_matrix_array(A, n, n, LOWER_BOUND, UPPER_BOUND, SEED);

        if (debug) {
            printf("[P%d] Generated matrix:\n", me);
            print_matrix_array(A, n, n);
            printf("\n");
            fflush(stdout);
        }
    }

    // calculate the number of rows to distribute
    // calculate number of elements and 
    // matrix offsets for every processor
    if (debug && me == MASTER) {
        printf(
            "[P%d] %d mod (%d - 1) is %d\n",
            me,
            n,
            nproc,
            (n % (nproc - 1))
        );
        printf("\n");
        fflush(stdout);
    }

    sendcounts = malloc(nproc * sizeof *sendcounts);
    recvcounts = malloc(nproc * sizeof *recvcounts);
    senddispls = malloc(nproc * sizeof *senddispls);
    recvdispls = malloc(nproc * sizeof *recvdispls);
    scatterv_gatherv_describers(
        sendcounts,
        senddispls,
        recvcounts,
        recvdispls,
        &local_rows,
        nproc,
        me,
        n
    );

    // add 1 row to last processor, 2 rows for other ones
    local_g_rows = (me != nproc - 1)?
        local_rows + 2:
        local_rows + 1;
    // print number of rows, cells and offsets
    if (debug) {
        if (me == MASTER) {
            for (int p = 0; p < nproc; p++)
            {
                printf("\n");
                printf(
                    "I'll send to P%d %d cells from cell %d\n",
                    p,
                    sendcounts[p],
                    senddispls[p]
                );
                printf(
                    "I'll get back from P%d %d cells from cell %d\n",
                    p,
                    recvcounts[p],
                    recvdispls[p]
                );
                printf("\n");
            }
        }

        printf(
            "[P%d] Local ghosted matrix will have %d rows\n",
            me,
            local_g_rows
        );
        printf("\n");
        fflush(stdout);
        // MPI_Pause(me, MASTER, COMM);
    }

    if (debug) {
        printf(
            "[P%d] I will take %d rows\n",
            me,
            local_g_rows
        );
        printf("\n");
        printf(
            "[P%d] Convergence threshold is %.3e\n",
            me,
            CONVERGENCE_THRESHOLD
        );
        printf("\n");
        fflush(stdout);
    }

    // calculate inner rows indexes
    // for row exchange among processors
    first_g_row = n;
    last_g_row = (sendcounts[me]/n - 2) * n;
    last_local_row = last_g_row + n;
    if (debug) {
        printf("[P%d] First significative row index: %d\n", me, first_g_row);
        printf("[P%d] Last significative row index: %d\n", me, last_g_row);
        printf("[P%d] Last ghosted row index: %d\n", me, last_local_row);
        printf("\n");
        fflush(stdout);
        // MPI_Pause(me, MASTER, COMM);
    }

    local_A_g = malloc(sendcounts[me] * sizeof *local_A_g);
    local_A_g_prime = malloc(sendcounts[me] * sizeof *local_A_g_prime);
    memset(local_A_g_prime, 0, sendcounts[me]);

    // distribute initial matrix slices to processes
    MPI_Scatterv(
        A, sendcounts, senddispls, MPI_DOUBLE,
        local_A_g, sendcounts[me], MPI_DOUBLE,
        MASTER, COMM
    );
    // no more need for counts and displacements arrays
    free(sendcounts);
    free(senddispls);

    // last row in last submatrix of last process
    // is not considered for row replacements
    // between local_A_g and local_A_g_prime
    if (me == nproc - 1) {
        for (int i = last_local_row; i < last_local_row + n; i++) {
            local_A_g_prime[i] = local_A_g[i];
        }
    }

    // apply Jacobi method over submatrices
    num_iterations = 0;
    t_start = MPI_Wtime();
    do {
        if (debug) {
            printf("[P%d] Local ghosted matrix:\n", me);
            print_matrix_array(local_A_g, local_g_rows, n);
            printf("\n");
            fflush(stdout);
        }

        // apply a single iteration
        jacobi_iteration(local_A_g, local_A_g_prime, local_g_rows, n);
        num_iterations++;

        if (debug) {
            printf("[P%d] Local prime matrix:\n", me);
            print_matrix_array(local_A_g_prime, local_g_rows, n);
            printf("\n");
            fflush(stdout);
        }

        // check for convergence test before reiterating
        local_diffnorm = convergence_check_g(
            local_A_g,
            local_A_g_prime,
            local_g_rows,
            n
        );
        if (debug) {
            printf(
                "[P%d] At iteration %d, my local convergence value is %.3e\n",
                me,
                num_iterations,
                local_diffnorm
            );
            fflush(stdout);
        }

        // evaluate convergence value from all processes
        MPI_Allreduce(
            &local_diffnorm, &diffnorm, 1,
            MPI_DOUBLE, MPI_SUM, COMM
        );
        diffnorm = sqrt(diffnorm);
        if (debug && me == MASTER) {
            printf(
                "[P%d] At iteration %d, global convergence value is %.3e\n",
                me,
                num_iterations,
                diffnorm
            );
            printf("\n");
            fflush(stdout);
        }

        if (debug) {
            printf("[P%d] Start row exchange...\n", me);
            fflush(stdout);
        }

        // make sure everyone performed an iteration
        // before exchanging rows
        MPI_Barrier(COMM);

        // send upper unghosted row
        // to previous process and
        // receive first row
        // from previous one
        // P0 has no previous one
        if (me != MASTER) {
            if (debug) {
                printf(
                    "[P%d] Exchanging with process %d\n",
                    me,
                    me - 1
                );
                fflush(stdout);
            }

            MPI_Sendrecv(
                // send first unghosted line to previous process
                &local_A_g_prime[first_g_row], n, MPI_DOUBLE, me - 1, TAG,
                // receive first line from previous process
                // and put it as my first ghosted line
                local_A_g_prime, n, MPI_DOUBLE, me - 1, TAG,
                COMM, &status
            );
        }

        // send lower unghosted row
        // to next process and
        // receive last row
        // from next one
        // Pnproc-1 has no next one
        if (me != nproc - 1) {
            if (debug) {
                printf(
                    "[P%d] Exchanging with process %d\n",
                    me,
                    me + 1
                );
                fflush(stdout);
            }

            MPI_Sendrecv(
                // send last unghosted line to next process
                &local_A_g_prime[last_g_row], n, MPI_DOUBLE, me + 1, TAG,
                // receive last line from next process
                // and put it as my last ghosted line
                &local_A_g_prime[last_local_row], n, MPI_DOUBLE, me + 1, TAG,
                COMM, &status
            );
        }

        if (debug) {
            printf("[P%d] Local prime matrix:\n", me);
            print_matrix_array(local_A_g_prime, local_g_rows, n);
            printf("\n");
            fflush(stdout);
            // pause for debug purposes
            // MPI_Pause(me, MASTER, COMM);
        }

        // swap matrices
        replace_partial(
            local_A_g,
            local_A_g_prime,
            n,
            first_g_row,
            last_local_row,
            me
        );

        if (debug) {
            printf("[P%d] After swap, local matrix is now:\n", me);
            print_matrix_array(local_A_g, local_g_rows, n);
            printf("\n");
            fflush(stdout);
            // pause for debug purposes
            // MPI_Pause(me, MASTER, COMM);
        }
    } while (
        diffnorm > CONVERGENCE_THRESHOLD &&
        num_iterations < MAX_ITERATIONS
    );

    // no more need for local prime matrix
    free(local_A_g_prime);

    // unghost local submatrices before recollection
    // unghosted submatrices have 2 rows less
    local_A = malloc(local_rows * n * sizeof *local_A);
    // memset(local_A, 0, local_rows * n);
    if (debug) {
        printf(
            "[P%d] Unghosting %dx%d local matrix from %dx%d ghosted one...\n",
            me,
            local_rows,
            n,
            local_g_rows,
            n
        );
        printf("[P%d] Local submatrix before unghosting:\n", me);
        print_matrix_array(local_A_g, local_g_rows, n);
        printf("\n");
        fflush(stdout);
        // MPI_Pause(me, MASTER, COMM);
    }
    for (int i = 0; i < local_rows; i++) {
        for (int j = 0; j < n; j++) {
            // offset is made of n
            local_A[i*n + j] = local_A_g[i*n + n + j];
        }
    }
    // no more need for local matrix
    free(local_A_g);

    if (debug) {
        printf("[P%d] Local submatrix after unghosting:\n", me);
        print_matrix_array(local_A, local_rows, n);
        printf("\n");
        fflush(stdout);
        // MPI_Pause(me, MASTER, COMM);
    }

    // at last, recollect submatrices
    MPI_Gatherv(
        local_A, recvcounts[me], MPI_DOUBLE,
        A, recvcounts, recvdispls, MPI_DOUBLE,
        MASTER, COMM
    );
    // no more need for local unghosted matrix
    // and count and displacement arrays
    free(local_A);
    free(recvcounts);
    free(recvdispls);
    if (debug && me == MASTER) {
        printf(
            "[P%d] After %d iteration, matrix is:\n",
            me,
            num_iterations
        );
        print_matrix_array(A, n, n);
        printf("\n");
        fflush(stdout);
    }

    t_end = MPI_Wtime() - t_start;
    if (debug) {
        printf("\n");
        fflush(stdout);
    }

    // free memory
    if (me == MASTER) {
        free(A);
    }

    if (me == MASTER) {
        printf(
            "[P%d] The solution took %d iterations and has an error of %.3e.\n",
            me,
            num_iterations,
            diffnorm
        );
        fflush(stdout);
    }

    // calculate elapsed time
    MPI_Reduce(
        &t_end, &t_max, 1, MPI_DOUBLE, MPI_MAX,
        MASTER, COMM
    );

    if (debug) {
        printf("[P%d] Local calculation time: %.3lf ms\n", me, t_end * MS_IN_S);
    }
    if (me == MASTER) {
        printf("[P%d] Max time: %.3f ms\n", me, t_max * MS_IN_S);
        printf("\n");
        printf("Writing result in %s\n", output_file);
        fflush(stdout);
        results = fopen(output_file, "a");
        fprintf(results, "%d,%f\n", n, t_max);
        fflush(results);
        // close file
        fclose(results);
    }

    // everyone, get rid of file name
    free(output_file);

    if (me == MASTER) {
        printf("\n\v%s terminated succesfully!\n", argv[0]);
    }
    if (debug && me != MASTER) {
        printf("\n\v%s terminated succesfully!\n", argv[0]);
    }

    MPI_Finalize();

    return EXIT_SUCCESS;
}
