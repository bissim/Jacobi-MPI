/**
 * @file jacobi-parallel.c
 * @ingroup runnable
 * @author Simone Bisogno (bissim.github.io)
 * @brief Parallel version of Jacobi method.
 * @version 0.1.0-beta+20200212
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
    int *sendcounts, *recvcounts;
    int *senddispls, *recvdispls;
    extern int MASTER; // TODO try nproc - 1;

    // time management variables
    double t_start;
    double t_end;
    double t_max;

    // program execution management
    unsigned char debug = 0;

    // business variables
    /**
     * @brief The order of the matrix associated to linear system
     *
     */
    int n;
    int local_rows;
    int local_g_rows;
    /**
     * @brief The number of rows distributed to every processor
     *
     */
    int rows_per_proc;
    /**
     * @brief The number of rows given to last processor if n/nproc
     * has got remainder
     *
     */
    int rem_rows;
    /**
     * @brief The coefficient matrix in linear system
     *
     */
    double *A;
    double *local_A;
    double *local_A_g;
    double *local_A_prime;
    int g_index;
    int num_iterations;
    double diffnorm;
    double local_diffnorm;

    printf("Running %s...\n\n\v", argv[0]);
    fflush(stdout);

    // check for command-line arguments
    if (argc < 2) {
		printf("\a\aInsufficient number of parameters!\n");
        printf("Usage: %s <matrixOrder> [<debugFlag>]\n\n", argv[0]);
        fflush(stdout);
		exit(EXIT_FAILURE);
	}
    else if (argc == 2) {
        n = atoi(argv[1]);
        debug = 0;
    }
	else {
		n = atoi(argv[1]);
        debug = (unsigned char) atoi(argv[2]);
	}
    printf("Matrix dimension: %dx%d (%d elements)\n", n, n, n*n);
    fflush(stdout);

    // initialize MPI environment
    MPI_Init(&argc, &argv);

    MPI_Comm_size(COMM, &nproc);
    MPI_Comm_rank(COMM, &me);

    // check whether number of processor is power of 2
    if ((nproc & (nproc - 1)) != 0) {
        fprintf(
            stderr,
            "\a[P%d] Number of processor must be a power of 2!",
            me
        );
        fflush(stderr);
        MPI_Abort(COMM, EXIT_FAILURE);
    }
    else if (n == nproc) {
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
        fflush(stderr);
        MPI_Abort(COMM, EXIT_FAILURE);
    }

    printf("\n\t\t\a>>> PROCESS %d OF %d <<<\n\n", me, nproc);
    fflush(stdout);

    // calculating the number of rows to distribute
    rows_per_proc = round(n / (double) nproc);
    rem_rows = (n % (nproc - 1)) != 0? n % (nproc - 1): rows_per_proc;
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
    // according to rows_per_proc rounding, increase remainder
    if (((double) rows_per_proc) < (n / (double) nproc)) {
        rem_rows++;
    }

    // printf(
    //     "[P%d] Distributing %d of %d matrix rows to %d processes\n",
    //     me,
    //     rows_per_proc,
    //     n,
    //     nproc
    // );
    // if (n % nproc != 0) {
    //     printf(
    //         "[P%d] P%d will get %d rows instead.\n\n",
    //         me,
    //         nproc - 1,
    //         rem_rows
    //     );
    // }
    // fflush(stdout);

    // generate matrix and known terms vector
    A = malloc(n * n * sizeof *A);
    if (me == MASTER) {
        printf("[P%d] Generating matrix...\n", me);
        printf("\n");
        fflush(stdout);
        generate_matrix_array(A, n, n, LOWER_BOUND, UPPER_BOUND, SEED);

        if (debug) {
            printf("[P%d] Generated matrix:\n", me);
            print_matrix_array(A, n, n);
            printf("\n");
            fflush(stdout);
        }
    }

    // calculate number of elements and 
    // matrix offsets for every processor
    sendcounts = malloc(nproc * sizeof *sendcounts);
    recvcounts = malloc(nproc * sizeof *recvcounts);
    senddispls = malloc(nproc * sizeof *senddispls);
    recvdispls = malloc(nproc * sizeof *recvdispls);
    for (int i = 0; i < nproc; i++) {
        if (i == MASTER || i == nproc - 1) {
            sendcounts[i] = (rows_per_proc + 1) * n;
            if (i == MASTER) {
                senddispls[i] = 0;
                recvdispls[i] = 0;
            }
            else {
                recvdispls[i] = (rows_per_proc) * i * n;
                senddispls[i] = recvdispls[i] - n;
            }
        }
        else {
            sendcounts[i] = (rows_per_proc + 2) * n;
            recvdispls[i] = (rows_per_proc) * i * n;
            senddispls[i] = recvdispls[i] - n;
        }
        recvcounts[i] = rows_per_proc * n;
    }
    // handle last rows for remainder
    if (rem_rows != rows_per_proc) {
        sendcounts[nproc - 1] = (rem_rows + 1) * n;
        recvcounts[nproc - 1] = rem_rows * n;
    }

    local_rows = (me != nproc - 1)?
        rows_per_proc:
        rem_rows;
    // add 1 row to first and last processor, 2 rows for other ones
    local_g_rows = (me != MASTER && me != nproc - 1)?
        local_rows + 2:
        local_rows + 1;

    // print number of rows, cells and offsets
    if (debug && me == MASTER) {
        for (int i = 0; i < nproc; i++) {
            printf(
                "[P%d] P%d will take %d cells starting from cell %d ",
                me,
                i,
                sendcounts[i],
                senddispls[i]
            );
            printf(
                "and will send back %d cells starting from cell %d.\n",
                recvcounts[i],
                recvdispls[i]
            );
        }
        printf("\n");
        fflush(stdout);
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
        fflush(stdout);
    }

    // apply Jacobi method
    local_A = malloc(local_rows * n * sizeof *local_A);
    local_A_g = malloc(sendcounts[me] * sizeof *local_A_g);
    // local_A_prime = calloc(local_rows * n, sizeof *local_A_prime);
    local_A_prime = calloc(sendcounts[me], sizeof *local_A_prime);
    num_iterations = 0;
    t_start = MPI_Wtime();
    do {
        // receive current iteration submatrix
        MPI_Scatterv(
            A, sendcounts, senddispls, MPI_DOUBLE,
            local_A_g, sendcounts[me], MPI_DOUBLE,
            MASTER, COMM
        );

        if (debug) {
            printf("[P%d] Received local ghosted matrix:\n", me);
            print_matrix_array(local_A_g, local_g_rows, n);
            printf("\n");
            fflush(stdout);
        }

        // apply a single iteration
        jacobi_iteration(local_A_g, local_A_prime, local_g_rows, n);
        num_iterations++;

        // check for convergence test before reiterating
        local_diffnorm = convergence_check(
            local_A_g,
            local_A_prime,
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
            printf("[P%d] Local prime matrix:\n", me);
            print_matrix_array(local_A_prime, local_g_rows, n);
            printf("\n");
            fflush(stdout);
        }

        // swap matrices
        // swap_pointers(&local_A, &local_A_prime);
        replace_elements(local_A_g, local_A_prime, local_g_rows, n);

        // unghost local_A_g
        g_index = (me == MASTER)? 0: n; // master includes first row
        for (int i = g_index; i < local_rows * n; i++) {
            local_A[i - g_index] = local_A_g[i];
        }
        if (debug) {
            printf("[P%d] Local matrix unghosted:\n", me);
            print_matrix_array(local_A, local_rows, n);
            printf("\n");
            fflush(stdout);
        }

        if (debug && me == MASTER) {
            printf("Press ENTER to continue...\n");
            getchar();
            fflush(stdout);
        }

        // at last, recollect submatrices
        MPI_Gatherv(
            local_A, local_rows*n, MPI_DOUBLE,
            A, recvcounts, recvdispls, MPI_DOUBLE,
            MASTER, COMM
        );
        if (debug && me == MASTER) {
            printf(
                "[P%d] After iteration %d, matrix is:\n",
                me,
                num_iterations
            );
            print_matrix_array(A, n, n);
            printf("\n");
            fflush(stdout);
        }
    } while (
        diffnorm > CONVERGENCE_THRESHOLD &&
        num_iterations < MAX_ITERATIONS
    );
    t_end = MPI_Wtime() - t_start;
    if (debug) {
        printf("\n");
        fflush(stdout);
    }

    // free memory
    if (me == MASTER) {
        free(A);
    }
    free(sendcounts);
    free(senddispls);
    free(recvcounts);
    free(recvdispls);
    free(local_A);
    free(local_A_prime);

    if (me == MASTER) {
        printf(
            "[P%d] The solution took %d iterations and has an error of %.3e.\n",
            me,
            num_iterations,
            diffnorm
        );
        printf("\n");
        fflush(stdout);
    }

    // calculate elapsed time
    MPI_Reduce(
        &t_end, &t_max, 1, MPI_DOUBLE, MPI_MAX,
        MASTER, COMM
    );

    printf("[P%d] Local calculation time: %.3lf ms\n", me, t_end * MS_IN_S);
    if (me == MASTER) {
        printf("[P%d] Max time: %.3f ms\n", me, t_max * MS_IN_S);
    }

    printf("\n\v\a\a\a%s terminated succesfully!\n", argv[0]);
    MPI_Finalize();

    return EXIT_SUCCESS;
}
