// jacobi
#include "definitions.h"

double sweep_jacobi(Plate* p, double** src, double** dst) {
    double local_err = 0.0;
    int i, j;

    double dx2 = p->dx * p->dx;
    int chunk = ROW_CHUNK;

#pragma omp for schedule(dynamic, chunk) nowait
    for (i = 1; i < GRID_SIZE - 1; i++) {
        const double* up = src[i - 1];
        const double* row = src[i];
        const double* down = src[i + 1];
        const double* f = p->f[i];
        double* out = dst[i];

        for (j = 1; j < GRID_SIZE - 1; j++) {
            out[j] = 0.25 * (up[j] + down[j] + row[j - 1] + row[j + 1] - (dx2 * f[j]));

            double diff = fabs(out[j] - row[j]);
            if (diff > local_err) local_err = diff;
        }
    }
    return local_err;
}
