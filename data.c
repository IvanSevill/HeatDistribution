#include "definitions.h"
#include <time.h>

void init_plate(Plate* p) {
    p->T = (double**)malloc(GRID_SIZE * sizeof(double*));
    p->T_new = (double**)malloc(GRID_SIZE * sizeof(double*));
    for (int i = 0; i < GRID_SIZE; i++) {
        p->T[i] = (double*)malloc(GRID_SIZE * sizeof(double));
        p->T_new[i] = (double*)malloc(GRID_SIZE * sizeof(double));
        for (int j = 0; j < GRID_SIZE; j++) p->T[i][j] = p->T_new[i][j] = 0.0;
    }

    for (int i = 0; i < GRID_SIZE; i++) {
        p->T[0][i] = p->T_new[0][i] = p->temp_up;
        p->T[GRID_SIZE - 1][i] = p->T_new[GRID_SIZE - 1][i] = p->temp_down;
        p->T[i][0] = p->T_new[i][0] = p->temp_left;
        p->T[i][GRID_SIZE - 1] = p->T_new[i][GRID_SIZE - 1] = p->temp_right;
    }
}

void free_plate(Plate* p) {
    if (p->T == NULL) return;
    for (int i = 0; i < GRID_SIZE; i++) {
        free(p->T[i]);
        free(p->T_new[i]);
    }
    free(p->T);
    free(p->T_new);
    p->T = NULL;
}

void save_results_to_csv(Plate* p, int iterations, double final_error, double time_seconds) {
    FILE* f = fopen("simulation_results.csv", "a");
    if (f == NULL) return;

    fseek(f, 0, SEEK_END);
    if (ftell(f) == 0) {
        fprintf(f, "Timestamp,Method,Grid_Size,Threads,Omega,Threshold,Iterations,Final_Error,Execution_Time_Sec\n");
    }

    time_t now = time(NULL);
    char* date = ctime(&now);
    date[strlen(date) - 1] = '\0';

    fprintf(f, "%s,%s,%d,%d,%.2f,%.6f,%d,%.8f,%.4f\n",
        date,
        (p->method == METHOD_SOR ? "SOR" : "Jacobi"),
        GRID_SIZE,      
        p->num_threads,
        p->omega,
        p->threshold,
        iterations,
        final_error,
        time_seconds);

    fclose(f);
}