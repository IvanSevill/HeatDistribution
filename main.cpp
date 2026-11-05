#include "definitions.h"
#ifndef HEAT_NO_GUI
#include "raylib.h"
#endif
#include <string.h>
#include <limits.h>
#include <cmath>

#ifndef _WIN32
#include <strings.h>
#define _stricmp strcasecmp
#endif

#define PROGRESS_INTERVAL 1000
#define DEFAULT_MAX_ITERATIONS 10000000
#define EXIT_NOT_CONVERGED 1
#define EXIT_USAGE 2

static void print_usage(FILE* out) {
    fprintf(out,
        "Usage: HeatSimulation_Parallel [options]\n"
        "  --no-gui             Run without a window\n"
        "  --method NAME        JACOBI (default), SOR, GAUSS or GAUSS-SEIDEL\n"
        "  --equation NAME      LAPLACE (default) or POISSON\n"
        "  --threads N          Thread count; above the OpenMP maximum uses the maximum\n"
        "  --threshold X        Stop when the largest change in a sweep is at or below X (> 0, default 0.001)\n"
        "  --omega X            SOR relaxation factor (0 < X < 2, default 1.7)\n"
        "  --max-iter N         Give up after N sweeps (default %d), headless only\n"
        "  --help               Show this help\n",
        DEFAULT_MAX_ITERATIONS);
}

static bool parse_int(const char* text, int* value) {
    char* end;
    long parsed = strtol(text, &end, 10);
    if (end == text || *end != '\0' || parsed < INT_MIN || parsed > INT_MAX) return false;
    *value = (int)parsed;
    return true;
}

static bool parse_double(const char* text, double* value) {
    char* end;
    double parsed = strtod(text, &end);
    if (end == text || *end != '\0' || !std::isfinite(parsed)) return false;
    *value = parsed;
    return true;
}

static int usage_error(const char* option, const char* value, const char* expected) {
    fprintf(stderr, "Error: invalid value '%s' for %s (expected %s)\n", value, option, expected);
    return EXIT_USAGE;
}

// Returns -1 when the program should go on, otherwise the exit code to stop with.
static int parse_args(int argc, char* argv[], Plate* p, bool* use_gui, int* max_iterations, int max_threads) {
    for (int i = 1; i < argc; i++) {
        const char* option = argv[i];

        if (strcmp(option, "--no-gui") == 0) { *use_gui = false; continue; }
        if (strcmp(option, "--help") == 0) { print_usage(stdout); return 0; }

        if (strcmp(option, "--threads") != 0 && strcmp(option, "--method") != 0 &&
            strcmp(option, "--threshold") != 0 && strcmp(option, "--omega") != 0 &&
            strcmp(option, "--equation") != 0 && strcmp(option, "--max-iter") != 0) {
            fprintf(stderr, "Error: unknown option '%s'\n", option);
            print_usage(stderr);
            return EXIT_USAGE;
        }
        if (i + 1 >= argc) {
            fprintf(stderr, "Error: %s needs a value\n", option);
            return EXIT_USAGE;
        }
        const char* value = argv[++i];

        if (strcmp(option, "--threads") == 0) {
            int threads;
            if (!parse_int(value, &threads) || threads < 1) return usage_error(option, value, "a positive integer");
            if (threads > max_threads) {
                fprintf(stderr, "Warning: --threads %d is above the OpenMP maximum, using %d\n", threads, max_threads);
                threads = max_threads;
            }
            p->num_threads = threads;
        }
        else if (strcmp(option, "--method") == 0) {
            if (_stricmp(value, "SOR") == 0) p->method = METHOD_SOR;
            else if (_stricmp(value, "GAUSS") == 0 || _stricmp(value, "GAUSS-SEIDEL") == 0) p->method = METHOD_GAUSS_SEIDEL;
            else if (_stricmp(value, "JACOBI") == 0) p->method = METHOD_JACOBI;
            else return usage_error(option, value, "JACOBI, SOR, GAUSS or GAUSS-SEIDEL");
        }
        else if (strcmp(option, "--threshold") == 0) {
            if (!parse_double(value, &p->threshold) || p->threshold <= 0.0) return usage_error(option, value, "a number above 0");
        }
        else if (strcmp(option, "--omega") == 0) {
            // Outside (0, 2) SOR diverges, and the error check would then report NaN as converged.
            if (!parse_double(value, &p->omega) || p->omega <= 0.0 || p->omega >= 2.0) return usage_error(option, value, "a number between 0 and 2, exclusive");
        }
        else if (strcmp(option, "--equation") == 0) {
            if (_stricmp(value, "POISSON") == 0) p->eq_type = EQ_POISSON;
            else if (_stricmp(value, "LAPLACE") == 0) p->eq_type = EQ_LAPLACE;
            else return usage_error(option, value, "LAPLACE or POISSON");
        }
        else {
            if (!parse_int(value, max_iterations) || *max_iterations < 1) return usage_error(option, value, "a positive integer");
        }
    }
    return -1;
}

static int run_headless_simulation(Plate* p, int max_iterations) {
    const char* method_name = (p->method == METHOD_JACOBI) ? "Jacobi" :
        (p->method == METHOD_SOR) ? "SOR" : "Gauss-Seidel";

    printf("\n[BENCHMARK] Method: %s | Threads: %d | Goal: %.10f\n",
        method_name, p->num_threads, p->threshold);

    omp_set_num_threads(p->num_threads);
    init_plate(p);

    int iterations = 0;
    double error = 1.0;
    double start_time = omp_get_wtime();

    while (error > p->threshold && iterations < max_iterations) {
        // Run up to the next progress line inside a single parallel region.
        int batch = PROGRESS_INTERVAL - iterations % PROGRESS_INTERVAL;
        if (batch > max_iterations - iterations) batch = max_iterations - iterations;
        iterations += solver_run(p, batch, &error);

        // Mostrar progreso cada 1000 iteraciones para no ralentizar con prints
        if (iterations % PROGRESS_INTERVAL == 0) {
            printf("  > Iter: %d | Current Max Error: %.10f\r", iterations, error);
            fflush(stdout); // Limpia el buffer para que se vea en la misma línea
        }
    }

    double total_time = omp_get_wtime() - start_time;
    bool converged = !(error > p->threshold);
    if (converged) {
        printf("\n[DONE] Converged in %d iterations. Time: %.4fs\n", iterations, total_time);
        save_results_to_csv(p, iterations, error, total_time);
    }
    else {
        printf("\n[STOPPED] No convergence after %d iterations (error %.10f). Time: %.4fs\n", iterations, error, total_time);
    }
    free_plate(p);
    return converged ? 0 : EXIT_NOT_CONVERGED;
}

int main(int argc, char* argv[]) {
    // 1. Initial Defaults
    int max_phys_threads = omp_get_max_threads();
    Plate p = { 100.0, 0.0, 0.0, 0.0, NULL, NULL, NULL, 0.1, METHOD_JACOBI, 1.7, 0.001, max_phys_threads, EQ_LAPLACE };
    bool use_gui = true;
    int max_iterations = DEFAULT_MAX_ITERATIONS;

    // 2. Parse Command Line Arguments
    int exit_code = parse_args(argc, argv, &p, &use_gui, &max_iterations, max_phys_threads);
    if (exit_code >= 0) return exit_code;

    if (!use_gui) {
        return run_headless_simulation(&p, max_iterations);
    }

#ifdef HEAT_NO_GUI
    fprintf(stderr, "Error: this build has no window, pass --no-gui\n");
    return EXIT_USAGE;
#else
    // 3. GUI MODE (Rest of your existing Raylib logic)
    AppState current_state = STATE_CONFIG;
    int iterations = 0;
    double last_error = 0.0, solver_time = 0.0;
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
            DrawText(TextFormat("5. Equation: %s (F)", (p.eq_type == EQ_LAPLACE ? "LAPLACE" : "POISSON")), 50, 300, 20, RED);
            if (IsKeyPressed(KEY_F)) {
                if (p.eq_type == EQ_LAPLACE) p.eq_type = EQ_POISSON;
                else p.eq_type = EQ_LAPLACE;
            }
            DrawRectangle(150, 450, 300, 60, DARKGREEN);
            DrawText("PRESS [ENTER] TO START", 185, 470, 20, RAYWHITE);
            if (IsKeyPressed(KEY_ENTER)) {
                omp_set_num_threads(p.num_threads);
                init_plate(&p);
                iterations = 0; data_saved = false;
                solver_time = 0.0;
                current_state = STATE_SIMULATING;
            }
            EndDrawing();
        }
        else {
            if (current_state == STATE_SIMULATING) {
                // Time the solver only: the frame rate paces the steps, not the computation.
                double step_start = omp_get_wtime();
                last_error = solver_step(&p);
                solver_time += omp_get_wtime() - step_start;
                iterations++;
                if (last_error <= p.threshold) {
                    current_state = STATE_FINISHED;
                }
            }
            if (current_state == STATE_FINISHED && !data_saved) {
                save_results_to_csv(&p, iterations, last_error, solver_time);
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
            DrawText(TextFormat("SOLVER TIME: %.4fs", solver_time), 20, WINDOW_SIZE + 15, 20, SKYBLUE);
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
#endif
}
