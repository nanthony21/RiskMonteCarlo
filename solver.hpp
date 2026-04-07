#pragma once
#include <tuple>
#include <vector>

std::tuple<std::vector<int>, std::vector<int>> solve_n_attacks(size_t N, int attackers, int defenders, bool atk_has_leader, bool def_has_leader);

std::tuple<int, int, bool> solve_attack(int attackers, int defenders, bool atk_has_leader, bool def_has_leader);

