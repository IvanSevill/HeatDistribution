#include "definitions.h"

double solve_sor(Plate* p) {
    double max_err = 0.0;
    int i, j, color;

#pragma omp parallel private(i, j, color)
    {
        double local_err = 0.0;
        for (color = 0; color < 2; color++) {
#pragma omp for nowait
            for (i = 1; i < GRID_SIZE - 1; i++) {
                for (j = 1; j < GRID_SIZE - 1; j++) {
                    if ((i + j) % 2 == color) {
                        double old_val = p->T[i][j];
                        double avg = 0.25 * (p->T[i - 1][j] + p->T[i + 1][j] + p->T[i][j - 1] + p->T[i][j + 1]);
                        p->T[i][j] = (1.0 - p->omega) * old_val + (p->omega * avg);
                        double diff = fabs(p->T[i][j] - old_val);
                        if (diff > local_err) local_err = diff;
                    }
                }
            }
#pragma omp barrier
        }
#pragma omp critical
        { if (local_err > max_err) max_err = local_err; }
    }
    return max_err;
}