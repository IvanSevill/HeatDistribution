// gauss-seidel
#include "definitions.h"
#include <math.h>

double sweep_gauss_seidel(Plate* p) {
    double local_err = 0.0;
    int i, j, color;

    double dx2 = p->dx * p->dx;
    double** T = p->T;

    for (color = 0; color < 2; color++) {
#pragma omp for schedule(dynamic, 16) nowait
        for (i = 1; i < GRID_SIZE - 1; i++) {
            const double* up = T[i - 1];
            const double* down = T[i + 1];
            const double* f = p->f[i];
            double* row = T[i];

            // First cell of this color in the row, then every second cell.
            for (j = ((i + 1) % 2 == color) ? 1 : 2; j < GRID_SIZE - 1; j += 2) {
                double old_val = row[j];

                row[j] = 0.25 * (up[j] + down[j] + row[j - 1] + row[j + 1] - (dx2 * f[j]));

                double diff = fabs(row[j] - old_val);
                if (diff > local_err) local_err = diff;
            }
        }
        // The second color's barrier is the caller's, which also publishes the error.
        if (color == 0) {
#pragma omp barrier
        }
    }
    return local_err;
}
