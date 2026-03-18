// jacobi
#include "definitions.h"

double solve_jacobi(Plate* p) {
    double max_err = 0.0;
    int i, j;

    double dx2 = p->dx * p->dx;

#pragma omp parallel private(i, j)
    {
        double local_err = 0.0;
#pragma omp for nowait
        for (i = 1; i < GRID_SIZE - 1; i++) {
            for (j = 1; j < GRID_SIZE - 1; j++) {
                p->T_new[i][j] = 0.25 * (p->T[i - 1][j] + p->T[i + 1][j] + p->T[i][j - 1] + p->T[i][j + 1] - (dx2 * p->f[i][j]));

                double diff = fabs(p->T_new[i][j] - p->T[i][j]);
                if (diff > local_err) local_err = diff;
            }
        }
#pragma omp critical
        { if (local_err > max_err) max_err = local_err; }
    }

#pragma omp parallel for private(i, j)
    for (i = 1; i < GRID_SIZE - 1; i++) {
        for (j = 1; j < GRID_SIZE - 1; j++) p->T[i][j] = p->T_new[i][j];
    }
    return max_err;
}