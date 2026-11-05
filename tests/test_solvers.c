// Solver tests with no dependencies. Exit code 0 means every check passed.
#include "definitions.h"

static int failures = 0;

#define CHECK(condition, ...) do { \
    if (!(condition)) { \
        failures++; \
        printf("FAIL %s:%d: ", __func__, __LINE__); \
        printf(__VA_ARGS__); \
        printf("\n"); \
    } \
} while (0)

static const SolveMethod METHODS[] = { METHOD_JACOBI, METHOD_GAUSS_SEIDEL, METHOD_SOR };
static const char* const METHOD_NAMES[] = { "Jacobi", "Gauss-Seidel", "SOR" };
#define METHOD_COUNT 3

static Plate make_plate(SolveMethod method, EquationType equation, double threshold) {
    Plate p;
    memset(&p, 0, sizeof(p));
    p.temp_up = 100.0;
    p.dx = 0.1;
    p.method = method;
    p.omega = 1.7;
    p.threshold = threshold;
    p.num_threads = 1;
    p.eq_type = equation;
    init_plate(&p);
    return p;
}

static void set_edge_cell(Plate* p, int i, int j, double value) {
    p->T[i][j] = value;
    p->T_new[i][j] = value;
}

static int same_field(const Plate* a, const Plate* b) {
    for (int i = 0; i < GRID_SIZE; i++) {
        if (memcmp(a->T[i], b->T[i], GRID_SIZE * sizeof(double)) != 0) return 0;
    }
    return 1;
}

static double max_distance(const Plate* p, double (*exact)(int, int)) {
    double worst = 0.0;
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            double distance = fabs(p->T[i][j] - exact(i, j));
            if (distance > worst) worst = distance;
        }
    }
    return worst;
}

// A linear field is an exact solution of the discrete Laplace equation.
static double linear_field(int i, int j) { return 3.0 * i - 2.0 * j + 5.0; }

// x^2 + y^2 is an exact solution of the discrete Poisson equation with f = 4.
static double quadratic_field(int i, int j) {
    double x = i * 0.1, y = j * 0.1;
    return x * x + y * y;
}

static void set_edges_from(Plate* p, double (*exact)(int, int)) {
    for (int k = 0; k < GRID_SIZE; k++) {
        set_edge_cell(p, 0, k, exact(0, k));
        set_edge_cell(p, GRID_SIZE - 1, k, exact(GRID_SIZE - 1, k));
        set_edge_cell(p, k, 0, exact(k, 0));
        set_edge_cell(p, k, GRID_SIZE - 1, exact(k, GRID_SIZE - 1));
    }
}

static void test_laplace_reaches_exact_solution(void) {
    for (int m = 0; m < METHOD_COUNT; m++) {
        Plate p = make_plate(METHODS[m], EQ_LAPLACE, 1e-11);
        set_edges_from(&p, linear_field);
        solver_run(&p, 10000000, NULL);
        double distance = max_distance(&p, linear_field);
        CHECK(distance < 1e-6, "%s is %g away from the exact solution", METHOD_NAMES[m], distance);
        free_plate(&p);
    }
}

static void test_poisson_reaches_exact_solution(void) {
    for (int m = 0; m < METHOD_COUNT; m++) {
        Plate p = make_plate(METHODS[m], EQ_LAPLACE, 1e-12);
        set_edges_from(&p, quadratic_field);
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) p.f[i][j] = 4.0;
        }
        solver_run(&p, 10000000, NULL);
        double distance = max_distance(&p, quadratic_field);
        CHECK(distance < 1e-6, "%s is %g away from the exact solution", METHOD_NAMES[m], distance);
        free_plate(&p);
    }
}

// Hand-computed first sweep below the hot top edge: tells the three methods apart.
static void test_first_sweep_values(void) {
    const double expected_1_1[METHOD_COUNT] = { 25.0, 25.0, 42.5 };
    const double expected_1_2[METHOD_COUNT] = { 25.0, 37.5, 78.625 };
    for (int m = 0; m < METHOD_COUNT; m++) {
        Plate p = make_plate(METHODS[m], EQ_LAPLACE, 0.0);
        solver_step(&p);
        CHECK(fabs(p.T[1][1] - expected_1_1[m]) < 1e-12, "%s T[1][1] = %g", METHOD_NAMES[m], p.T[1][1]);
        CHECK(fabs(p.T[1][2] - expected_1_2[m]) < 1e-12, "%s T[1][2] = %g", METHOD_NAMES[m], p.T[1][2]);
        free_plate(&p);
    }
}

static void test_sor_with_omega_one_is_gauss_seidel(void) {
    Plate gs = make_plate(METHOD_GAUSS_SEIDEL, EQ_LAPLACE, 1e-5);
    Plate sor = make_plate(METHOD_SOR, EQ_LAPLACE, 1e-5);
    sor.omega = 1.0;
    int gs_iterations = solver_run(&gs, 10000000, NULL);
    int sor_iterations = solver_run(&sor, 10000000, NULL);
    CHECK(gs_iterations == sor_iterations, "%d against %d iterations", gs_iterations, sor_iterations);
    CHECK(same_field(&gs, &sor), "fields differ");
    free_plate(&gs);
    free_plate(&sor);
}

static void test_thread_count_does_not_change_results(void) {
    const int thread_counts[] = { 2, 3, 8 };
    const EquationType equations[] = { EQ_LAPLACE, EQ_POISSON };
    for (int m = 0; m < METHOD_COUNT; m++) {
        for (int e = 0; e < 2; e++) {
            omp_set_num_threads(1);
            Plate reference = make_plate(METHODS[m], equations[e], 1e-4);
            double reference_error;
            int reference_iterations = solver_run(&reference, 10000000, &reference_error);

            for (int t = 0; t < 3; t++) {
                omp_set_num_threads(thread_counts[t]);
                Plate p = make_plate(METHODS[m], equations[e], 1e-4);
                double error;
                int iterations = solver_run(&p, 10000000, &error);
                CHECK(iterations == reference_iterations, "%s, %d threads: %d iterations, expected %d",
                    METHOD_NAMES[m], thread_counts[t], iterations, reference_iterations);
                CHECK(error == reference_error, "%s, %d threads: final error differs", METHOD_NAMES[m], thread_counts[t]);
                CHECK(same_field(&p, &reference), "%s, %d threads: field differs", METHOD_NAMES[m], thread_counts[t]);
                free_plate(&p);
            }
            free_plate(&reference);
        }
    }
    omp_set_num_threads(1);
}

// The window advances with solver_step, headless runs with solver_run. Odd and even
// sweep counts both matter because Jacobi alternates its two buffers.
static void test_stepping_matches_running(void) {
    const int sweep_counts[] = { 1, 2, 7, 8 };
    for (int m = 0; m < METHOD_COUNT; m++) {
        for (int c = 0; c < 4; c++) {
            Plate stepped = make_plate(METHODS[m], EQ_POISSON, 0.0);
            Plate run = make_plate(METHODS[m], EQ_POISSON, 0.0);
            double step_error = 0.0, run_error = 0.0;
            for (int s = 0; s < sweep_counts[c]; s++) step_error = solver_step(&stepped);
            int iterations = solver_run(&run, sweep_counts[c], &run_error);
            CHECK(iterations == sweep_counts[c], "%s ran %d sweeps", METHOD_NAMES[m], iterations);
            CHECK(step_error == run_error, "%s, %d sweeps: errors differ", METHOD_NAMES[m], sweep_counts[c]);
            CHECK(same_field(&stepped, &run), "%s, %d sweeps: fields differ", METHOD_NAMES[m], sweep_counts[c]);
            free_plate(&stepped);
            free_plate(&run);
        }
    }
}

static void test_iteration_cap_is_respected(void) {
    for (int m = 0; m < METHOD_COUNT; m++) {
        Plate p = make_plate(METHODS[m], EQ_LAPLACE, 1e-5);
        double error = 0.0;
        int iterations = solver_run(&p, 3, &error);
        CHECK(iterations == 3, "%s ran %d sweeps", METHOD_NAMES[m], iterations);
        CHECK(error > p.threshold, "%s reports error %g after 3 sweeps", METHOD_NAMES[m], error);
        free_plate(&p);
    }
}

static void test_edges_stay_fixed(void) {
    for (int m = 0; m < METHOD_COUNT; m++) {
        Plate p = make_plate(METHODS[m], EQ_LAPLACE, 0.0);
        solver_run(&p, 5, NULL);
        for (int k = 1; k < GRID_SIZE - 1; k++) {
            CHECK(p.T[0][k] == 100.0, "%s top edge changed at %d", METHOD_NAMES[m], k);
            CHECK(p.T[GRID_SIZE - 1][k] == 0.0, "%s bottom edge changed at %d", METHOD_NAMES[m], k);
        }
        free_plate(&p);
    }
}

#if GRID_SIZE == 48
// Iteration counts of the original solvers, to catch any change in results.
static void test_iteration_counts_match_baseline(void) {
    const int laplace[METHOD_COUNT] = { 4076, 2194, 443 };
    const int poisson[METHOD_COUNT] = { 5630, 2971, 575 };
    for (int m = 0; m < METHOD_COUNT; m++) {
        Plate a = make_plate(METHODS[m], EQ_LAPLACE, 1e-5);
        Plate b = make_plate(METHODS[m], EQ_POISSON, 1e-5);
        int laplace_iterations = solver_run(&a, 10000000, NULL);
        int poisson_iterations = solver_run(&b, 10000000, NULL);
        CHECK(laplace_iterations == laplace[m], "%s Laplace: %d", METHOD_NAMES[m], laplace_iterations);
        CHECK(poisson_iterations == poisson[m], "%s Poisson: %d", METHOD_NAMES[m], poisson_iterations);
        free_plate(&a);
        free_plate(&b);
    }
}
#endif

int main(void) {
    omp_set_num_threads(1);
    test_laplace_reaches_exact_solution();
    test_poisson_reaches_exact_solution();
    test_first_sweep_values();
    test_sor_with_omega_one_is_gauss_seidel();
    test_thread_count_does_not_change_results();
    test_stepping_matches_running();
    test_iteration_cap_is_respected();
    test_edges_stay_fixed();
#if GRID_SIZE == 48
    test_iteration_counts_match_baseline();
#endif

    if (failures > 0) {
        printf("test_solvers: %d checks failed\n", failures);
        return 1;
    }
    printf("test_solvers: all checks passed\n");
    return 0;
}
