/**
 * @defgroup runnable Runnable sources
 * @file jacobi-serial.c
 * @ingroup runnable
 * @author Simone Bisogno (bissim.github.io)
 * @brief Serial version of Jacobi method.
 * @version 0.1.0-beta+20200302
 * @date 2020-02-03
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
#include <time.h>

#include "matrixutils.h"
#include "jacobi.h"
#include "misc.h"

/**
 * @brief How many nanoseconds in a second.
 * 
 * How many nanoseconds in a second.
 */
extern const long NS_IN_S;
/**
 * @brief How many milliseconds in a second.
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
 * @brief The main function of Jacobi method in serial version.
 *
 * @param argc Count of command-line parameters
 * @param argv Command-line parameters
 * @return int Return value indicating whether program execution succeded
 */
int main(int argc, char** argv) {
    // local variables
    /**
     * @brief The order of the matrix associated to linear system
     *
     */
    int n;
    /**
     * @brief The coefficient matrix in linear system
     *
     */
    double *A;
    int num_iterations;
    double err;
    double elapsedtime;
    struct timespec start, stop;
    unsigned char debug = 0;

    printf("Running %s...\n\n\v", argv[0]);
    fflush(stdout);

    // reading dimension and debug flag from command line
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
    printf("\n");
    fflush(stdout);

    // allocate memory for matrix and vectors
    A = malloc(n * n * sizeof *A);

    // generate matrix
    generate_matrix_array(A, n, n, LOWER_BOUND, UPPER_BOUND, SEED);

    if (debug) {
        printf("Generated matrix:\n");
        print_matrix_array(A, n, n);
        printf("\n");
        fflush(stdout);
    }

    // apply Jacobi method
    clock_gettime(CLOCK_REALTIME, &start);
    num_iterations = jacobi(A, n, n, &err);
    clock_gettime(CLOCK_REALTIME, &stop);

    if (debug) {
        printf("Resulting matrix:\n");
        print_matrix_array(A, n, n);
        printf("\n");
        fflush(stdout);
    }

    free(A);

    printf("The solution took %d iterations ", num_iterations);
    printf("and has an error of %.3e.\n", err);
    fflush(stdout);
    // if (num_iterations == MAX_ITERATIONS) {
    //     fprintf(stderr, "\aSolution did not converge!\n");
    //     exit(EXIT_FAILURE);
    // }

    elapsedtime = (stop.tv_sec - start.tv_sec) +
        (stop.tv_nsec - start.tv_nsec) /
        (double) NS_IN_S;
    printf("Elapsed time: %f ms.\n", elapsedtime * MS_IN_S);
    fflush(stdout);

    printf("\n\v%s terminated succesfully!\n", argv[0]);
    return EXIT_SUCCESS;
}
