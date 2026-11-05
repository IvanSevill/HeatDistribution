#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <string.h>

#ifndef GRID_SIZE
#define GRID_SIZE 48
#endif
#define WINDOW_SIZE 600
// Rows a thread takes at a time. Small enough for every thread to get work on small grids,
// larger on big grids where a thread taking rows more often costs more than it balances.
#define ROW_CHUNK (GRID_SIZE < 256 ? 4 : 16)

typedef enum { METHOD_JACOBI, METHOD_SOR, METHOD_GAUSS_SEIDEL } SolveMethod;
typedef enum { STATE_CONFIG, STATE_SIMULATING, STATE_FINISHED } AppState;
typedef enum { EQ_LAPLACE, EQ_POISSON } EquationType;


typedef struct {
    double temp_up, temp_down, temp_left, temp_right;
    double** T;
    double** T_new;
    double** f;
    double dx;
    SolveMethod method;
    double omega;
    double threshold;
    int num_threads;
    EquationType eq_type;
} Plate;

#ifdef __cplusplus
extern "C" {
#endif

    void init_plate(Plate* p);
    void free_plate(Plate* p);
    // Runs up to max_iterations sweeps inside one parallel region and stops early once
    // the largest change in a sweep is at or below p->threshold. Returns the sweeps run.
    int solver_run(Plate* p, int max_iterations, double* final_error);
    // Runs one sweep and returns its largest change.
    double solver_step(Plate* p);
    // Per-thread sweeps: call from inside a parallel region. Each returns the largest change
    // in the cells this thread updated and leaves the final barrier to the caller.
    // Rows are scheduled dynamically so slower cores on hybrid CPUs do not hold the team at
    // each barrier; which thread updates a row does not affect the result.
    double sweep_jacobi(Plate* p, double** src, double** dst);
    double sweep_sor(Plate* p);
    double sweep_gauss_seidel(Plate* p);
    void save_results_to_csv(Plate* p, int iterations, double final_error, double time_seconds);

#ifdef __cplusplus
}
#endif

#endif
