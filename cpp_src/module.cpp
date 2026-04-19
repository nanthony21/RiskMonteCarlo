#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "solver.hpp"

PYBIND11_MODULE(risk_solver, m) {
    m.def("solve_attack", &solve_attack, "Function that solves");
    m.def("solve_n_attacks", &solve_n_attacks, "Function that solves");
}