#include <vector>
#include <array>
#include <random>
#include <ranges>
#include <algorithm>
#include <cassert>
#include "solver.hpp"
#include <tbb/tbb.h>
#include <chrono>
#include <future>

template<int N>
std::vector<std::array<uint8_t, N>> initialize_possible_rolls() {
    std::vector<std::array<uint8_t, N>> out;
    constexpr uint8_t SIDES = 6;
    std::array<uint8_t, N> this_combo;

    auto gen_rolls = [&out, &this_combo](this auto&& self, uint8_t dice_idx)  -> void {
        for (uint8_t dice_roll=1; dice_roll<=SIDES; dice_roll++) {
            this_combo[dice_idx] = dice_roll;
            if (dice_idx + 1 < N) {
                self(dice_idx + 1);
            }
            else {
                out.push_back(this_combo);
            }
        }
    };
    gen_rolls(0);
    for (auto& roll_combo : out) {
        // sort rolls
        std::ranges::sort(roll_combo, std::greater());
    }
    return out;
}

static auto ROLLS_1 = initialize_possible_rolls<1>();
static auto ROLLS_2 = initialize_possible_rolls<2>();
static auto ROLLS_3 = initialize_possible_rolls<3>();

thread_local std::default_random_engine gen(std::chrono::system_clock::now().time_since_epoch().count());

auto reserve_all = [](size_t N, auto&... vecs) {
    (vecs.reserve(N), ...); // fold expression (C++17)
};

std::tuple<std::vector<int>, std::vector<int>> solve_n_attacks(size_t N, int attackers, int defenders, bool atk_has_leader, bool def_has_leader) {
    std::vector<int> attackers_remain_g;
    std::vector<int> defenders_remain_g;

    reserve_all(N, attackers_remain_g, defenders_remain_g);
    std::mutex mut;

    tbb::parallel_for(tbb::blocked_range<size_t>(0, N, N / 16), [&attackers_remain_g, &defenders_remain_g, &mut, &attackers, &defenders, atk_has_leader, def_has_leader](tbb::blocked_range<size_t> const& rng) {
        std::vector<int> attackers_remain;
        std::vector<int> defenders_remain;
        reserve_all(rng.size(), attackers_remain, defenders_remain);

        for (auto it = rng.begin(); it!=rng.end(); ++it) {
            auto [a,b,c] = solve_attack(attackers, defenders, atk_has_leader, def_has_leader);
             attackers_remain.emplace_back(a);
             defenders_remain.emplace_back(b);
        }
        {
            std::scoped_lock lock(mut);
            attackers_remain_g.insert(attackers_remain_g.end(), attackers_remain.begin(), attackers_remain.end());
            defenders_remain_g.insert(defenders_remain_g.end(), defenders_remain.begin(), defenders_remain.end());
        }
    });
    // auto myTask = [] {
    //     auto [a,b,c] = solve_attack(attackers, defenders, atk_has_leader, def_has_leader);
    //     attackers_remain.emplace_back(a);
    //     defenders_remain.emplace_back(b);
    // };
    // auto handle = std::async(std::launch::async, myTask, 42);
    // handle.wait();
    // for (size_t i=0; i<N; i++) {
    //     auto [a,b,c] = solve_attack(attackers, defenders, atk_has_leader, def_has_leader);
    //     attackers_remain.emplace_back(a);
    //     defenders_remain.emplace_back(b);
    // }
    return {attackers_remain_g, defenders_remain_g};
}

 std::tuple<int, int, bool> solve_attack(int attackers, int defenders, bool atk_has_leader, bool def_has_leader) {
    while (true) {
        std::variant<std::array<uint8_t, 1>, std::array<uint8_t, 2>, std::array<uint8_t, 3>> att_roll;
        if (attackers >= 4) {
            auto rolls = ROLLS_3;
            const size_t idx = std::uniform_int_distribution<size_t>(0z, rolls.size() - 1)(gen);
            att_roll = rolls[idx];
        }
        else if (attackers == 3) {
            auto rolls = ROLLS_2;
            const size_t idx = std::uniform_int_distribution<size_t>(0z, rolls.size() - 1)(gen);
            att_roll = rolls[idx];
        }
        else {
            assert(attackers == 2);
            auto rolls = ROLLS_1;
            const size_t idx = std::uniform_int_distribution<size_t>(0z, rolls.size() - 1)(gen);
            att_roll = rolls[idx];
        }

        std::variant<std::array<uint8_t, 1>, std::array<uint8_t, 2>> def_roll;
        if (defenders >= 2) {
            auto rolls = ROLLS_2;
            const size_t idx = std::uniform_int_distribution<size_t>(0z, rolls.size() - 1)(gen);
            def_roll = rolls[idx];
        }
        else {
            assert(defenders == 1);
            auto rolls = ROLLS_1;
            const size_t idx = std::uniform_int_distribution<size_t>(0z, rolls.size() - 1)(gen);
            def_roll = rolls[idx];
        }

        // auto [atk_lose, def_lose] = std::visit([&def_roll, atk_has_leader, def_has_leader](auto& att_roll) -> std::tuple<int, int> {
            // return std::visit([atk_has_leader, def_has_leader, &att_roll](auto& def_roll) -> std::tuple<int, int> {
            //     if (atk_has_leader)
            //         ++(att_roll[0]);
            //     if (def_has_leader)
            //         ++(def_roll[0]);
            //
            //     int atk_lose = 0;
            //     int def_lose = 0;
            //     for (size_t i = 0 ; i < std::min(att_roll.size(), def_roll.size()); ++i) {
            //         if (att_roll[i] > def_roll[i]) {
            //             ++def_lose;
            //         }
            //         else {
            //             ++atk_lose;
            //         }
            //     }
            //     return {atk_lose, def_lose};
            //         }, def_roll);
            //     }
            //         , att_roll);
        auto [atk_lose, def_lose] = std::visit([atk_has_leader, def_has_leader](auto& att_roll, auto& def_roll) -> std::tuple<int, int> {
           if (atk_has_leader)
               ++(att_roll[0]);
           if (def_has_leader)
               ++(def_roll[0]);

           int atk_lose = 0;
           int def_lose = 0;
           for (size_t i = 0 ; i < std::min(att_roll.size(), def_roll.size()); ++i) {
               if (att_roll[i] > def_roll[i]) {
                   ++def_lose;
               }
               else {
                   ++atk_lose;
               }
           }
           return {atk_lose, def_lose};
        }, att_roll, def_roll);

        // Process rolls
        defenders -= def_lose;
        attackers -= atk_lose;
        if ((defenders <= 0) or (attackers <= 1)) {
            assert(defenders >= 0);
            assert(attackers >= 1);
            return {
                attackers,
                defenders,
                defenders == 0
            };
        }
    }
}


