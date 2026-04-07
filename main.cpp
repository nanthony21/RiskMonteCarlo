#include <print>

#include "solver.hpp"
int main() {
    auto [attackers, defenders, atk_wins] = solve_attack(30, 20, false, false);
    solve_n_attacks(1000, 20, 20, false, false);
    std::print("{} {} {}", attackers, defenders, atk_wins);
    return 0;
}