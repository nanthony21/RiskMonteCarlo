#include <print>
#include <ostream>
#include "solver.hpp"
int main() {
    // auto [attackers, defenders, atk_wins] = solve_attack(30, 20, false, false);
    solve_n_attacks(1000, 15, 21, false, false);
    // std::print("{} {} {}", attackers, defenders, atk_wins);
    return 0;
}