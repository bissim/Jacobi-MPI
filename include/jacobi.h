/**
 * @defgroup headers Header files.
 * @file jacobi.h
 * @ingroup headers
 * @author Simone Bisogno (bissim.github.io)
 * @brief Header file for Jacobi method functions.
 * @version 0.1.0-beta+20200327.1435
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

/**
 * @brief Maximum number of iterations for Jacobi method.
 */
static const short MAX_ITERATIONS = 1E2; // 100
/**
 * @brief Minimum error required to Jacobi method.
 */
static const double CONVERGENCE_THRESHOLD = 1E-2; // 0.01

int jacobi(double *, int, int, double *);
void jacobi_iteration(double *, double *, int, int);
void swap_pointers(void **, void **);
void replace_elements(double *, double *, int, int);
void replace_partial(double *, double *, int, int, int, int);
double convergence_check_g(double *, double *, int, int);
double convergence_check(double *, double *, int, int);
void scatterv_gatherv_describers(
    int *,
    int *,
    int *,
    int *,
    int *,
    int,
    int,
    int
);

#ifdef __cplusplus
}
#endif

#endif // JACOBI_H_
