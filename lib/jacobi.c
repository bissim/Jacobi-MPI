/**
 * @defgroup libraries Library sources
 * @file jacobi.c
 * @ingroup libraries
 * @author Simone Bisogno (bissim.github.io)
 * @brief Jacobi method functions.
 * @version 0.1.0-rc.3+20200406
 * @date 2020-02-04
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#define _XOPEN_SOURCE 700 /**< Use timespec definition from POSIX */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "matrixutils.h"
#include "jacobi.h"
#include "mpiutils.h"

extern const short MAX_ITERATIONS; /**< Maximum number of iterations allowed */
extern const double CONVERGENCE_THRESHOLD; /**< Error threshold */

/**
 * @brief Jacobi method function.
 * 
 * @param A Input matrix
 * @param rows Number of input matrix rows
 * @param columns Number of input matrix columns
 * @param eps Error in applicating Jacobi method
 * @return int Number of Jacobi method iterations
 */
int jacobi(double *A, int rows, int columns, double *eps) {
    int itr;
    double diff;
    double *A_prime;

    itr = 0;
    A_prime = malloc(rows * columns * sizeof *A_prime);
    memset(A_prime, 0, rows * columns);
    do {
        jacobi_iteration(A, A_prime, rows, columns);
        itr++;
        // calculate convergence value
        diff = convergence_check_g(A, A_prime, rows, columns);
        diff = sqrt(diff);
        // swap matrices
        // swap_pointers(&A, &A_prime);
        replace_elements(A, A_prime, rows, columns);
        // debug printing
        // printf("Matrix at iteration %d:\n", itr);
        // print_matrix_array(A, rows, columns);
        // printf("Press ENTER to continue...\n");
        // fflush(stdout);
        // getchar();
    } while (diff > CONVERGENCE_THRESHOLD && itr < MAX_ITERATIONS);
    free(A_prime);
    *eps = diff;

    return itr;
}

/**
 * @brief A single iteration of Jacobi method.
 * 
 * @param A Input matrix
 * @param A_prime The 'A' matrix after Jacobi iteration
 * @param rows Number of input matrix rows
 * @param columns Number of input matrix columns
 */
void jacobi_iteration(double *A, double *A_prime, int rows, int columns) {

    for (int i = 1; i < rows - 1; i++) {
        for (int j = 1; j < columns - 1; j++) {
            // to perform following check, 'i' and 'j'
            // have to be in range [0, rows|columns]
            // if (i == 0 || j == 0 || i == rows || j == columns) {
            //     A_prime[i*columns + j] = A[i*columns + j];
            // }
            // else {
                // we're considering 'columns' instead of 'rows'
                // because it retains the original value of number of
                // rows in the original matrix
                // TL;DR 'tis the correct offsetting
                A_prime[i*columns + j] = (
                    A[(i+1)*columns + j] +
                    A[(i-1)*columns + j] +
                    A[i*columns + j+1] +
                    A[i*columns + j-1]
                )/4.0;
            // }
        }
    }

}

/**
 * @brief Swap matrix (as array) pointers
 * 
 * @param a Pointer to a matrix (as array) 
 * @param b Pointer to a matrix (as array)
 */
void swap_pointers(void **a, void **b) {
    /**
     * @brief Temporary pointer for swap
     * 
     * Temporary pointer for swap
     */
    void *temp;

    temp = *a;
    *a = *b;
    *b = temp;
}

/**
 * @brief Swap matrix (as array) elements one by one but border ones
 * 
 * @param a A matrix (as array)
 * @param b A matrix (as array)
 * @param rows Number of input matrix rows
 * @param columns Number of input matrix columns
 */
void replace_elements(double *a, double *b, int rows, int columns) {
    for (int i = 1; i < rows - 1; i++) {
        for (int j = 1; j < columns - 1; j++) {
            a[i * columns + j] = b[i * columns + j];
        }
    }
}

/**
 * @brief Swap matrix (as array) elements one by one from first element
 * (or from 0 if process is 0) to last element
 * 
 * @param a A matrix (as array)
 * @param b A matrix (as array)
 * @param columns Number of input matrix columns
 * @param first_element First element to swap
 * @param last_element Last row element to swap
 * @param process Process providing the matrix array
 */
void replace_partial(
    double *a,
    double *b,
    int columns,
    int first_element,
    int last_element,
    int process
) {
    if (process != 0) {
        first_element = 0;
    }

    for (int i = first_element + 1; i < last_element + columns - 1; i++) {
        // skip replace if I'm on border
        if (i % columns == 0 || i % columns == columns - 1) continue;
        a[i] = b[i];
    }
}

/**
 * @brief Calculate the error of a single Jacobi iteration for ghosted matrix
 * 
 * @param x The matrix before Jacobi iteration 
 * @param x_prime The matrix after Jacobi iteration
 * @param rows Number of input matrix rows
 * @param columns Number of input matrix columns
 * @return double Error of a single Jacobi iteration
 */
double convergence_check_g(double *x, double *x_prime, int rows, int columns) {
    double diff = 0.0;

    for (int i = 1; i < rows - 1; i++) {
        for (int j = 1; j < columns - 1; j++) {
            // see jacobi_iteration function for the reason why
            // 'columns' is used instead of 'rows'
            diff += (x_prime[i*columns+j] - x[i*columns+j]) *
                (x_prime[i*columns+j] - x[i*columns+j]);
        }
    }

    return diff;
}

/**
 * @brief Calculate the error of a single Jacobi iteration for non-ghosted matrix
 * 
 * @param x The matrix before Jacobi iteration 
 * @param x_prime The matrix after Jacobi iteration
 * @param rows Number of input matrix rows
 * @param columns Number of input matrix columns
 * @return double Error of a single Jacobi iteration
 */
double convergence_check(double *x, double *x_prime, int rows, int columns) {
    double diff = 0.0;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < columns; j++) {
            // see jacobi_iteration function for the reason why
            // 'columns' is used instead of 'rows'
            diff += (x_prime[i*columns+j] - x[i*columns+j]) *
                (x_prime[i*columns+j] - x[i*columns+j]);
        }
    }

    return diff;
}

void scatterv_gatherv_describers(
    int *scounts,
    int *sdispls,
    int *rcounts,
    int *rdispls,
    int *local_rows,
    int nproc,
    int pid,
    int dim
) {
    // calculating the number of rows to distribute
    int rows_per_proc = round(dim / (double) nproc);
    int rem_rows = (dim % (nproc - 1)) != 0? dim % (nproc - 1): rows_per_proc;

    // according to rows_per_proc rounding, increase remainder
    if (((double) rows_per_proc) < (dim / (double) nproc)) {
        rem_rows++;
    }

    // printf(
    //     "Distributing %d of %d matrix rows to %d processes\n",
    //     rows_per_proc,
    //     dim,
    //     nproc
    // );
    // if (dim % nproc != 0) {
    //     printf(
    //         "P%d will get %d rows instead.\n",
    //         nproc - 1,
    //         rem_rows
    //     );
    //     printf("\n");
    // }
    // fflush(stdout);

    for (int i = 0; i < nproc; i++) {
        if (i == MASTER || i == nproc - 1) {
            scounts[i] = (rows_per_proc + 1) * dim;
            if (i == MASTER) {
                sdispls[i] = 0;
                rdispls[i] = dim;
            }
            else {
                rdispls[i] = (rows_per_proc) * i * dim;
                sdispls[i] = rdispls[i] - dim;
            }
        }
        else {
            scounts[i] = (rows_per_proc + 2) * dim;
            rdispls[i] = (rows_per_proc) * i * dim;
            sdispls[i] = rdispls[i] - dim;
        }
        rcounts[i] = scounts[i] - 2 * dim;
    }
    // handle last rows for remainder
    if (rem_rows != rows_per_proc) {
        scounts[nproc - 1] = (rem_rows + 1) * dim;
        rcounts[nproc - 1] = rem_rows * dim;
    }

    *local_rows = (pid != nproc - 1)?
        rows_per_proc:
        rem_rows;
    // MASTER has 1 row less
    *local_rows = (pid != MASTER)?
        *local_rows:
        *local_rows - 1;

}
