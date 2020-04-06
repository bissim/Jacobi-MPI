/**
 * @file matrixutils.c
 * @ingroup libraries
 * @author Simone Bisogno (bissim.github.io)
 * @brief Matrix utility functions.
 * @version 0.1.0-rc.3+20200406
 * @date 2020-02-07
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "matrixutils.h"

/**
 * @brief Generate a matrix as array.
 * 
 * Generate a matrix as array.
 */
void generate_matrix_array(
    double *v,
    int rows,
    int columns,
    double min,
    double max,
    int seed
) {
    srand(seed);

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < columns; j++) {
            v[i*rows + j] = min + (rand() / (RAND_MAX / (max - min)));
        }
    }
}

/**
 * @brief Generate a dominant diagonal matrix as array.
 * 
 * Generate a dominant diagonal matrix as array.
 */
void generate_dd_matrix_array(
    double *v,
    int rows,
    int columns,
    double min,
    double max,
    int seed
) {
    int pivot_index = 0;
    int n = (rows + columns) / 2;
    srand(seed);

    for (int i = 0; i < rows*columns; i++) {
        if (i == n*pivot_index + pivot_index) {
            v[i] = pow(min + (rand() / (RAND_MAX / (max - min))), 2);
            pivot_index++;
        }
        else {
            v[i] = min + (rand() / (RAND_MAX / (max - min)));
        }
    }
}

/**
 * @brief Print an array matrix.
 * 
 * Print an array matrix.
 */
void print_matrix_array(double *array, int rows, int columns) {
    if (fmax((float) rows, (float) columns) > 50 || rows * columns > 100) {
        printf("\tToo large to represent (%d elements)!\n", rows * columns);
        return;
    }

    for (int i = 0; i < rows * columns; ++i) {
        printf("%8.3f\t", array[i]);
        if (i % columns == columns - 1) {
            printf("\n");
        }
    }
}

/**
 * @brief Copy existing array matrix into another one.
 * 
 * Copy existing array matrix into another one.
 */
void copy_matrix_array(double *array, double *copy, int rows, int columns) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < columns; j++) {
            copy[i * columns + j] = array[i * columns + j];
        }
    }
}

/**
 * @brief Array matrix-vector dot product.
 * 
 * Array matrix-vector dot product.
 */
void prod_mat_vett(double result[], double *a, int rows, int cols, double v[]) {
    for (int i = 0; i < rows; i++) {
        result[i] = 0;
        for (int j = 0; j < cols; j++) {
            result[i] += a[i*cols+j] * v[j];
        }
    }
}

/**
 * @brief Transpose an array matrix.
 * 
 * Transpose an array matrix.
 */
void transpose_matrix_array(
    double *array,
    double *transpose,
    int rows,
    int columns
) {
    int source_position, destination_position;
    float source;

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
            source_position = columns * i + j;
            destination_position = rows * j + i;
            source = *(array + source_position);
            *(transpose + destination_position) = source;
        }
    }
}
