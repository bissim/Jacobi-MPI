/**
 * @file matrixutils.c
 * @author Simone Bisogno (bissim.github.io)
 * @brief Matrix utility functions.
 * @version 0.1.0-beta+20200207
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
 * Funzione che genera una matrice come array
 */
void generate_matrix_array(double *v, int rows, int columns, double min, double max, int seed) {
    srand(seed);

    for (int i = 0; i < rows*columns; i++) {
        v[i] = min + (rand() / (RAND_MAX / (max - min)));
    }
}

/**
 * Funzione che genera una matrice come array
 * Genera matrici diagonali dominanti
 */
void generate_dd_matrix_array(double *v, int rows, int columns, double min, double max, int seed) {
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
 * Funzione che esegue la stampa di una matrice
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
 * Funzione che esegue il prodotto matrice vettore
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
 * Funzione che esegue la trasposizione di una matrice
 */
void transpose_matrix_array(double *array, double *transpose, int rows, int columns) {
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
