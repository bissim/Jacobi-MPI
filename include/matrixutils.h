/**
 * @file matrixutils.h
 * @author Simone Bisogno (bissim.github.io)
 * @brief Header file for Matrix utility functions.
 * @version 0.1.0-beta+20200207
 * @date 2020-02-07
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef MATRIXUTILS_H_
#define MATRIXUTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

void generate_matrix_array(double *, int, int, double, double, int);
void generate_dd_matrix_array(double *, int, int, double, double, int);
void print_matrix_array(double *, int, int);
void prod_mat_vett(double *, double *, int, int, double *);
void transpose_matrix_array(double *, double *, int, int);

#ifdef __cplusplus
}
#endif

#endif // MATRIXUTILS_H_
