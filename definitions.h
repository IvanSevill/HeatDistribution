#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <string.h>

#define GRID_SIZE 48
#define WINDOW_SIZE 600

typedef enum { METHOD_JACOBI, METHOD_SOR, METHOD_GAUSS_SEIDEL } SolveMethod;
typedef enum { STATE_CONFIG, STATE_SIMULATING, STATE_FINISHED } AppState;


typedef struct {
    double temp_up, temp_down, temp_left, temp_right;
    double** T;
    double** T_new;
    SolveMethod method;
    double omega;
    double threshold;
    int num_threads;
} Plate;

#ifdef __cplusplus
extern "C" {
#endif

    void init_plate(Plate* p);
    void free_plate(Plate* p);
    double solver_step(Plate* p);
    double solve_jacobi(Plate* p);
    double solve_sor(Plate* p);
    double solve_gauss_seidel(Plate* p);
    void save_results_to_csv(Plate* p, int iterations, double final_error, double time_seconds);

#ifdef __cplusplus
}
#endif

#endif