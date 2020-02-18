/**
 * @file matrixutils.h
 * @ingroup headers
 * @author Simone Bisogno (bissim.github.io)
 * @brief Header file for Matrix utility functions.
 * @version 0.1.0-beta+20200212
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

/**
 * @brief Generate a matrix as array.
 * 
 * Generate a matrix as array.
 */
void generate_matrix_array(double *, int, int, double, double, int);
/**
 * @brief Generate a matrix as array.
 * 
 * Generate a matrix as array.
 */
void generate_matrix_array_par(double *, int, int, double, double, int);
/**
 * @brief Generate a dominant diagonal matrix as array.
 * 
 * Generate a dominant diagonal matrix as array.
 */
void generate_dd_matrix_array(double *, int, int, double, double, int);
/**
 * @brief Print an array matrix.
 * 
 * Print an array matrix.
 */
void print_matrix_array(double *, int, int);
/**
 * @brief Array matrix-vector dot product.
 * 
 * Array matrix-vector dot product.
 */
void prod_mat_vett(double *, double *, int, int, double *);
/**
 * @brief Transpose an array matrix.
 * 
 * Transpose an array matrix.
 */
void transpose_matrix_array(double *, double *, int, int);

#ifdef __cplusplus
}
#endif

#endif // MATRIXUTILS_H_
