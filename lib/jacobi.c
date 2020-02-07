/**
 * @file jacobi.c
 * @author Simone Bisogno (bissim.github.io)
 * @brief Jacobi method functions.
 * @version 0.1.0-beta+20200207
 * @date 2020-02-04
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#define _XOPEN_SOURCE 700 /**< Use timespec definition from POSIX */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "matrixutils.h"
#include "jacobi.h"

extern const short MAX_ITERATIONS; /**< Maximum number of iterations allowed */
extern const double CONVERGENCE_THRESHOLD; /**< Error threshold */

/**
 * Jacobi solver function.
 */
int jacobi(double *A, int rows, int columns, double *eps) { // TODO check
    int itr;
    double diff;
    double *A_prime;

    itr = 0;
    A_prime = calloc(rows * columns, sizeof *A_prime);
    do {
        jacobi_iteration(A, A_prime, rows, columns);
        itr++;
        // calculate convergence value
        diff = convergence_check(A, A_prime, rows, columns);
        diff = sqrt(diff);
        // swap pointers
        // swap_pointers(&A, &A_prime); // TODO check
        replace_elements(A, A_prime, rows, columns);
        // printf("Matrix after iteration %d:\n", itr);
        // print_matrix_array(A, rows, columns);
        // printf("Error: %f\n", diff);
        // printf("Press a key to continue...");
        // getchar();
        // printf("\n");
        // fflush(stdout);
    } while (diff > CONVERGENCE_THRESHOLD && itr < MAX_ITERATIONS);
    free(A_prime);
    *eps = diff;

    return itr;
}

void jacobi_iteration(double *A, double *A_prime, int rows, int columns) {
    for (int i = 1; i < rows - 1; i++) {
        for (int j = 1; j < columns - 1; j++) {
            A_prime[i*rows + j] = (
                A[(i+1)*rows + j] +
                A[(i-1)*rows + j] +
                A[i*rows + j+1] +
                A[i*rows + j-1]
            )/4;
        }
    }
}

void swap_pointers(double **a, double **b) {
    double *temp;

    temp = *a;
    *a = *b;
    *b = temp;
}

void replace_elements(double *a, double *b, int rows, int columns) {
    for (int i = 1; i < rows - 1; i++) {
        for (int j = 1; j < columns - 1; j++) {
            a[i * rows + j] = b[i * rows + j];
        }
    }
}

double convergence_check(double *x, double *x_prime, int rows, int columns) {
    double diff = 0.0;

    for (int i = 1; i < rows - 1; i++) {
        for (int j = 1; j < columns - 1; j++) {
            diff += (x_prime[i*rows+j] - x[i*rows+j]) *
                (x_prime[i*rows+j] - x[i*rows+j]);
        }
    }

    return diff;
}
