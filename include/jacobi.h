/**
 * @file jacobi.h
 * @author Simone Bisogno (bissim.github.io)
 * @brief Header file for Jacobi method functions.
 * @version 0.1.0-beta+20200207
 * @date 2020-02-07
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef JACOBI_H_
#define JACOBI_H_

#ifdef __cplusplus
extern "C" {
#endif

static const short MAX_ITERATIONS = 1E2; // 100
static const double CONVERGENCE_THRESHOLD = 1E-2; // 0.01

int jacobi(double *, int, int, double *);
void jacobi_iteration(double *, double *, int, int);
void swap_pointers(double **, double **);
void replace_elements(double *, double *, int, int);
double convergence_check(double *, double *, int, int);

#ifdef __cplusplus
}
#endif

#endif // JACOBI_H_
