#include "definitions.h"
#include "raylib.h"
#include <string.h>

void run_headless_simulation(Plate* p) {
    const char* method_name = (p->method == METHOD_JACOBI) ? "Jacobi" :
        (p->method == METHOD_SOR) ? "SOR" : "Gauss-Seidel";

    printf("\n[BENCHMARK] Method: %s | Threads: %d | Goal: %.10f\n",
        method_name, p->num_threads, p->threshold);

    omp_set_num_threads(p->num_threads);
    init_plate(p);

    int iterations = 0;
    double error = 1.0;
    double start_time = omp_get_wtime();

    while (error > p->threshold) {
        error = solver_step(p);
        iterations++;

        // Mostrar progreso cada 1000 iteraciones para no ralentizar con prints
        if (iterations % 1000 == 0) {
            printf("  > Iter: %d | Current Max Error: %.10f\r", iterations, error);
            fflush(stdout); // Limpia el buffer para que se vea en la misma línea
        }
    }

    double total_time = omp_get_wtime() - start_time;
    printf("\n[DONE] Converged in %d iterations. Time: %.4fs\n", iterations, total_time);
    save_results_to_csv(p, iterations, error, total_time);
    free_plate(p);
}

int main(int argc, char* argv[]) {
    // 1. Initial Defaults
    int max_phys_threads = omp_get_max_threads();
    Plate p = { 100.0, 0.0, 0.0, 0.0, NULL, NULL, METHOD_JACOBI, 1.7, 0.001, max_phys_threads };
    bool use_gui = true;

    // 2. Parse Command Line Arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-gui") == 0) use_gui = false;

        if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) {
            int val = atoi(argv[++i]);
            if (val > 0 && val <= max_phys_threads) p.num_threads = val;
        }

        if (strcmp(argv[i], "--method") == 0 && i + 1 < argc) {
            i++; 
            if (_stricmp(argv[i], "SOR") == 0) p.method = METHOD_SOR;
            else if (_stricmp(argv[i], "GAUSS") == 0) p.method = METHOD_GAUSS_SEIDEL;
            else p.method = METHOD_JACOBI;
        }

        if (strcmp(argv[i], "--threshold") == 0 && i + 1 < argc) {
            p.threshold = atof(argv[++i]);
        }

        if (strcmp(argv[i], "--omega") == 0 && i + 1 < argc) {
            p.omega = atof(argv[++i]);
        }
    }

    if (!use_gui) {
        run_headless_simulation(&p);
        return 0;
    }

    // 3. GUI MODE (Rest of your existing Raylib logic)
    AppState current_state = STATE_CONFIG;
    int iterations = 0;
    double last_error = 0.0, start_time = 0.0, total_time = 0.0;
    bool data_saved = false;

    InitWindow(WINDOW_SIZE, WINDOW_SIZE + 100, "Parallel Heat Simulator");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (current_state == STATE_CONFIG) {
            const char* gui_method_name = (p.method == METHOD_JACOBI) ? "JACOBI" :
                (p.method == METHOD_SOR) ? "SOR" : "GAUSS-SEIDEL";

            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText("SIMULATION PARAMETERS", 140, 40, 25, DARKGRAY);
            DrawText(TextFormat("1. Method: %s (J/S/G)", gui_method_name), 50, 100, 20, BLACK);
            if (IsKeyPressed(KEY_J)) p.method = METHOD_JACOBI;          
            if (IsKeyPressed(KEY_S)) p.method = METHOD_SOR;
            if (IsKeyPressed(KEY_G)) p.method = METHOD_GAUSS_SEIDEL; 
            DrawText(TextFormat("2. Target Error: %.5f (Q/E)", p.threshold), 50, 150, 20, BLACK);
            if (IsKeyPressed(KEY_E)) p.threshold /= 10.0;
            if (IsKeyPressed(KEY_Q)) p.threshold *= 10.0;
            DrawText(TextFormat("3. Threads: %d (+/-)", p.num_threads), 50, 200, 20, DARKBLUE);
            if (IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_PERIOD)) { if (p.num_threads < max_phys_threads) p.num_threads++; }
            if (IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_COMMA)) { if (p.num_threads > 1) p.num_threads--; }
            DrawText(TextFormat("4. Temps: U:%.0f D:%.0f L:%.0f R:%.0f (ZXCV)", p.temp_up, p.temp_down, p.temp_left, p.temp_right), 50, 250, 20, MAROON);
            if (IsKeyDown(KEY_Z)) p.temp_up += 1; if (IsKeyDown(KEY_X)) p.temp_down += 1;
            if (IsKeyDown(KEY_C)) p.temp_left += 1; if (IsKeyDown(KEY_V)) p.temp_right += 1;
            if (IsKeyDown(KEY_LEFT_SHIFT)) {
                if (IsKeyDown(KEY_Z)) p.temp_up -= 2; if (IsKeyDown(KEY_X)) p.temp_down -= 2;
                if (IsKeyDown(KEY_C)) p.temp_left -= 2; if (IsKeyDown(KEY_V)) p.temp_right -= 2;
            }
            DrawRectangle(150, 450, 300, 60, DARKGREEN);
            DrawText("PRESS [ENTER] TO START", 185, 470, 20, RAYWHITE);
            if (IsKeyPressed(KEY_ENTER)) {
                omp_set_num_threads(p.num_threads);
                init_plate(&p);
                iterations = 0; data_saved = false;
                start_time = omp_get_wtime();
                current_state = STATE_SIMULATING;
            }
            EndDrawing();
        }
        else {
            if (current_state == STATE_SIMULATING) {
                last_error = solver_step(&p);
                iterations++;
                if (last_error <= p.threshold) {
                    total_time = omp_get_wtime() - start_time;
                    current_state = STATE_FINISHED;
                }
            }
            if (current_state == STATE_FINISHED && !data_saved) {
                save_results_to_csv(&p, iterations, last_error, total_time);
                data_saved = true;
            }
            BeginDrawing();
            ClearBackground(BLACK);
            int cell_size = WINDOW_SIZE / GRID_SIZE;
            for (int i = 0; i < GRID_SIZE; i++) {
                for (int j = 0; j < GRID_SIZE; j++) {
                    float t = (float)p.T[i][j] / 100.0f;
                    if (t > 1.0f) t = 1.0f; if (t < 0.0f) t = 0.0f;
                    Color c = { (unsigned char)(t * 255), 50, (unsigned char)((1.0f - t) * 255), 255 };
                    DrawRectangle(j * cell_size, i * cell_size, cell_size, cell_size, c);
                }
            }
            DrawRectangle(0, WINDOW_SIZE, WINDOW_SIZE, 100, DARKGRAY);
            DrawText(TextFormat("TIME: %.4fs", (current_state == STATE_SIMULATING ? omp_get_wtime() - start_time : total_time)), 20, WINDOW_SIZE + 15, 20, SKYBLUE);
            DrawText(TextFormat("THREADS: %d | ITERS: %d", p.num_threads, iterations), 20, WINDOW_SIZE + 45, 18, LIGHTGRAY);
            DrawText(TextFormat("ERR: %.6f | GOAL: %.5f", last_error, p.threshold), 280, WINDOW_SIZE + 15, 20, YELLOW);
            if (current_state == STATE_FINISHED) {
                DrawRectangle(150, 200, 300, 100, Fade(GREEN, 0.8f));
                DrawText("CONVERGED!", 210, 225, 30, WHITE);
            }
            if (IsKeyPressed(KEY_R)) { free_plate(&p); current_state = STATE_CONFIG; }
            EndDrawing();
        }
    }
    if (current_state != STATE_CONFIG) free_plate(&p);
    CloseWindow();
    return 0;
}