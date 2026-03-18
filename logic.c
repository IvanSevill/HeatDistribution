#include "definitions.h"

double solver_step(Plate* p) {
    switch (p->method) {
    case METHOD_JACOBI: return solve_jacobi(p);
    case METHOD_SOR: return solve_sor(p);
    case METHOD_GAUSS_SEIDEL: return solve_gauss_seidel(p); 
    }
    return 0.0;
}