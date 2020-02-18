/**
 * @defgroup libraries Library sources
 * @file jacobi.c
 * @ingroup libraries
 * @author Simone Bisogno (bissim.github.io)
 * @brief Jacobi method functions.
 * @version 0.1.0-beta+20200212
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
#include <omp.h>

#include "matrixutils.h"
#include "jacobi.h"

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
    A_prime = calloc(rows * columns, sizeof *A_prime);
    do {
        jacobi_iteration(A, A_prime, rows, columns);
        itr++;
        // calculate convergence value
        diff = convergence_check_g(A, A_prime, rows, columns);
        diff = sqrt(diff);
        // swap matrices
        // swap_pointers(&A, &A_prime);
        replace_elements(A, A_prime, rows, columns);
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
            A_prime[i*rows + j] = (
                A[(i+1)*rows + j] +
                A[(i-1)*rows + j] +
                A[i*rows + j+1] +
                A[i*rows + j-1]
            )/4;
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
 * @brief Swap matrix (as array) elements one by one
 * 
 * @param a A matrix (as array)
 * @param b A matrix (as array)
 * @param rows Number of input matrix rows
 * @param columns Number of input matrix columns
 */
void replace_elements(double *a, double *b, int rows, int columns) {
    #pragma omp parallel for
    for (int i = 1; i < rows - 1; i++) {
        for (int j = 1; j < columns - 1; j++) {
            a[i * rows + j] = b[i * rows + j];
        }
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

    #pragma omp parallel for reduction(+: diff)
    for (int i = 1; i < rows - 1; i++) {
        for (int j = 1; j < columns - 1; j++) {
            diff += (x_prime[i*rows+j] - x[i*rows+j]) *
                (x_prime[i*rows+j] - x[i*rows+j]);
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

    #pragma omp parallel for reduction(+: diff)
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < columns; j++) {
            diff += (x_prime[i*rows+j] - x[i*rows+j]) *
                (x_prime[i*rows+j] - x[i*rows+j]);
        }
    }

    return diff;
}
