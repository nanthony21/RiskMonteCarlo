#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "solver.hpp"

PYBIND11_MODULE(risk_solver, m) {
    m.def("solve_attack", &solve_attack, R"doc(
 Perform one randomized attack up until the point that one side is victorious.

Args:
    attackers: The starting number of attackers
    defenders: The starting number of defenders
    atk_has_leader: Whether or not the attackers have a "leader". This adds 1 to the highest attacker dice roll
    def_has_leader: Whether or not the defenders have a "leader". This adds 1 to the highest defender dice roll
    def_is_capitol: If the defender is a capitol then the attackers can only attack with 2 dice

Returns:
    The number of remaining attackers, the number of remaining defenders
)doc");

    m.def("solve_n_attacks", &solve_n_attacks, R"doc(
Run `solve_attack` N times and return histograms of the results.

Args:
    N: The number of trials to run
    attackers: The starting number of attackers
    defenders: The starting number of defenders
    atk_has_leader: Whether or not the attackers have a "leader". This adds 1 to the highest attacker dice roll
    def_has_leader: Whether or not the defenders have a "leader". This adds 1 to the highest defender dice roll
    def_is_capitol: If the defender is a capitol then the attackers can only attack with 2 dice

Returns:
    Histogram of the times that attackers won and histogram of times that defenders won
)doc");
}