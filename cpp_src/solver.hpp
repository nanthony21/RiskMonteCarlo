#pragma once
#include <tuple>
#include <vector>

/**
 * Run `solve_attack` N times and return histograms of the results.
 *
 * @param N The number of trials to run
 * @param attackers The starting number of attackers
 * @param defenders The starting number of defenders
 * @param atk_has_leader Whether or not the attackers have a "leader". This adds 1 to the highest attacker dice roll
 * @param def_has_leader Whether or not the defenders have a "leader". This adds 1 to the highest defender dice roll
 * @return
 */
std::tuple<std::vector<size_t>, std::vector<size_t>>  solve_n_attacks(size_t N, int attackers, int defenders, bool atk_has_leader, bool def_has_leader);

/**
 * Perform one randomized attack up until the point that one side is victorious.
 *
 * @param attackers The starting number of attackers
 * @param defenders The starting number of defenders
 * @param atk_has_leader Whether or not the attackers have a "leader". This adds 1 to the highest attacker dice roll
 * @param def_has_leader Whether or not the defenders have a "leader". This adds 1 to the highest defender dice roll
 * @return The number of remaining attackers, the number of remaining defenders, whether attackers won.
 */
std::tuple<int, int, bool> solve_attack(int attackers, int defenders, bool atk_has_leader, bool def_has_leader);

