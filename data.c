#include "definitions.h"
#include <time.h>

void init_plate(Plate* p) {
    p->T = (double**)malloc(GRID_SIZE * sizeof(double*));
    p->T_new = (double**)malloc(GRID_SIZE * sizeof(double*));
    p->f = (double**)malloc(GRID_SIZE * sizeof(double*));

    for (int i = 0; i < GRID_SIZE; i++) {
        p->T[i] = (double*)malloc(GRID_SIZE * sizeof(double));
        p->T_new[i] = (double*)malloc(GRID_SIZE * sizeof(double));
        p->f[i] = (double*)malloc(GRID_SIZE * sizeof(double));

        
        for (int j = 0; j < GRID_SIZE; j++) {
            p->T[i][j] = p->T_new[i][j] = 0.0;
            p->f[i][j] = 0.0; 
        }
    }

    for (int i = 0; i < GRID_SIZE; i++) {
        p->T[0][i] = p->T_new[0][i] = p->temp_up;
        p->T[GRID_SIZE - 1][i] = p->T_new[GRID_SIZE - 1][i] = p->temp_down;
        p->T[i][0] = p->T_new[i][0] = p->temp_left;
        p->T[i][GRID_SIZE - 1] = p->T_new[i][GRID_SIZE - 1] = p->temp_right;
    }

    if (p->eq_type == EQ_POISSON) {
        int center_start = GRID_SIZE / 2 - (GRID_SIZE / 10);
        int center_end = GRID_SIZE / 2 + (GRID_SIZE / 10);

        if (center_start == center_end) {
            center_start--;
            center_end++;
        }

        for (int i = center_start; i < center_end; i++) {
            for (int j = center_start; j < center_end; j++) {
                p->f[i][j] = -10000.0;
            }
        }
    }
}

void free_plate(Plate* p) {
    if (p->T == NULL) return;
    for (int i = 0; i < GRID_SIZE; i++) {
        free(p->T[i]);
        free(p->T_new[i]);
        free(p->f[i]);
    }
    free(p->T);
    free(p->T_new);
    free(p->f);
    p->T = NULL;
}

void save_results_to_csv(Plate* p, int iterations, double final_error, double time_seconds) {
    FILE* f = fopen("simulation_results.csv", "a");
    if (f == NULL) return;

    fseek(f, 0, SEEK_END);
    if (ftell(f) == 0) {
        fprintf(f, "Timestamp,Equation,Method,Grid_Size,Threads,Omega,Threshold,Iterations,Final_Error,Execution_Time_Sec\n");
    }

    time_t now = time(NULL);
    char* date = ctime(&now);
    date[strlen(date) - 1] = '\0';

    const char* method_name = (p->method == METHOD_JACOBI) ? "Jacobi" :
        (p->method == METHOD_SOR) ? "SOR" : "Gauss-Seidel";

    const char* eq_name = (p->eq_type == EQ_POISSON) ? "Poisson" : "Laplace";

    fprintf(f, "%s,%s,%s,%d,%d,%.2f,%.6f,%d,%.8f,%.4f\n",
        date,
        eq_name,     
        method_name,
        GRID_SIZE,
        p->num_threads,
        p->omega,
        p->threshold,
        iterations,
        final_error,
        time_seconds);

    fclose(f);
}