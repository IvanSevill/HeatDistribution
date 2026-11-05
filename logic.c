#include "definitions.h"

static double thread_sweep(Plate* p, int iteration) {
    switch (p->method) {
    case METHOD_JACOBI:
        // Jacobi alternates its two buffers instead of copying one into the other.
        return (iteration % 2 == 0) ? sweep_jacobi(p, p->T, p->T_new) : sweep_jacobi(p, p->T_new, p->T);
    case METHOD_SOR: return sweep_sor(p);
    case METHOD_GAUSS_SEIDEL: return sweep_gauss_seidel(p);
    }
    return 0.0;
}

int solver_run(Plate* p, int max_iterations, double* final_error) {
    int iterations = 0;
    double error = 0.0;

    // Two slots per thread, one for odd sweeps and one for even sweeps. A Jacobi sweep has no
    // barrier of its own, so a fast thread can already be publishing the next sweep's error
    // while a slow thread still reads this one; they must not share a slot or the threads
    // could decide differently and deadlock.
    double* errors = (double*)malloc(2 * omp_get_max_threads() * sizeof(double));
    if (errors == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        exit(EXIT_FAILURE);
    }

#pragma omp parallel
    {
        int thread = omp_get_thread_num();
        int team = omp_get_num_threads();
        int iteration = 0;
        double max_err = 0.0;
        int done = (max_iterations <= 0);

        while (!done) {
            int phase = iteration % 2;
            errors[2 * thread + phase] = thread_sweep(p, iteration);
            iteration++;

            // After this barrier this sweep's slots are final, so all threads reach the same decision.
#pragma omp barrier
            max_err = 0.0;
            for (int t = 0; t < team; t++) {
                if (errors[2 * t + phase] > max_err) max_err = errors[2 * t + phase];
            }
            done = (max_err <= p->threshold) || (iteration >= max_iterations);
        }

        if (thread == 0) {
            iterations = iteration;
            error = max_err;
        }
    }
    free(errors);

    // An odd number of Jacobi sweeps leaves the newest values in T_new.
    if (p->method == METHOD_JACOBI && iterations % 2 == 1) {
        double** newest = p->T_new;
        p->T_new = p->T;
        p->T = newest;
    }

    if (final_error != NULL) *final_error = error;
    return iterations;
}

double solver_step(Plate* p) {
    double error = 0.0;
    solver_run(p, 1, &error);
    return error;
}
