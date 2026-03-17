#include "definitions.h"

double solver_step(Plate* p) {
    if (p->method == METHOD_SOR) {
        return solve_sor(p);
    }
    else {
        return solve_jacobi(p);
    }
}