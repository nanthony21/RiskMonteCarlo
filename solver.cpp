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

// thread_local std::default_random_engine gen(std::chrono::system_clock::now().time_since_epoch().count());

std::default_random_engine& get_thread_local_engine() {
    thread_local std::default_random_engine gen(std::random_device {}());
    return gen;
}

auto reserve_all = [](size_t N, auto&... vecs) {
    (vecs.reserve(N), ...); // fold expression (C++17)
};

template<typename T, typename return_t>
struct Solver {
    // using return_t = typename T::ret_t;

    return_t aggregate() = delete;

    void append(int a, int d) = delete;

    std::tuple<return_t, return_t> solve_n_base(size_t N, int attackers, int defenders, bool atk_has_leader, bool def_has_leader) {
        std::mutex mut;

        tbb::parallel_for(tbb::blocked_range<size_t>(0, N, N / 16), [this, &mut, &attackers, &defenders, atk_has_leader, def_has_leader](tbb::blocked_range<size_t> const& rng) {
            for (auto it = rng.begin(); it!=rng.end(); ++it) {
                auto [a,b,c] = solve_attack(attackers, defenders, atk_has_leader, def_has_leader);
                static_cast<T*>(this)->append(a, b);
            }
        });

        {
            return static_cast<T*>(this)->aggregate();
        }
    }
};

// auto empty_hist = [](int count){std::vector<std::atomic<size_t>> v(count, 0); return v;};

struct HistogramSolver: Solver<HistogramSolver, std::vector<size_t>> {

    using hist_t = std::vector<size_t>;
    // using ret_t = typename hist_t;

    HistogramSolver(int attacker_count, int defender_count):
        atk_count(attacker_count+1),
        def_count(defender_count+1) {
        std::ranges::fill(atk_count, 0);
        std::ranges::fill(def_count, 0);
    }

    std::tuple<hist_t, hist_t> aggregate() {
        hist_t out_a;
        hist_t out_d;
        out_a.reserve(atk_count.size());
        out_d.reserve(def_count.size());
        for (auto const& cnt : atk_count) {
            out_a.push_back(cnt);
        }
        for (auto const& cnt : def_count) {
            out_d.push_back(cnt);
        }
        return {out_a, out_d};
    }

    void append(int a, int d) {
        if (a > 1) {
            atk_count[a]++;
        }
        else
            def_count[d]++;
    }
    std::vector<std::atomic<size_t>> atk_count;
    std::vector<std::atomic<size_t>> def_count;
};



std::tuple<std::vector<size_t>, std::vector<size_t>> solve_n_attacks(size_t N, int attackers, int defenders, bool atk_has_leader, bool def_has_leader) {
    auto [att, def] = HistogramSolver(attackers, defenders).solve_n_base(N, attackers, defenders, atk_has_leader, def_has_leader);
    return std::tuple {att, def};
}

 std::tuple<int, int, bool> solve_attack(int attackers, int defenders, bool atk_has_leader, bool def_has_leader) {
    while (true) {
        std::variant<std::array<uint8_t, 1>, std::array<uint8_t, 2>, std::array<uint8_t, 3>> att_roll;
        if (attackers >= 4) {
            auto rolls = ROLLS_3;
            const size_t idx = std::uniform_int_distribution<size_t>(0z, rolls.size() - 1)(get_thread_local_engine());
            att_roll = rolls[idx];
        }
        else if (attackers == 3) {
            auto rolls = ROLLS_2;
            const size_t idx = std::uniform_int_distribution<size_t>(0z, rolls.size() - 1)(get_thread_local_engine());
            att_roll = rolls[idx];
        }
        else {
            assert(attackers == 2);
            auto rolls = ROLLS_1;
            const size_t idx = std::uniform_int_distribution<size_t>(0z, rolls.size() - 1)(get_thread_local_engine());
            att_roll = rolls[idx];
        }

        std::variant<std::array<uint8_t, 1>, std::array<uint8_t, 2>> def_roll;
        if (defenders >= 2) {
            auto rolls = ROLLS_2;
            const size_t idx = std::uniform_int_distribution<size_t>(0z, rolls.size() - 1)(get_thread_local_engine());
            def_roll = rolls[idx];
        }
        else {
            assert(defenders == 1);
            auto rolls = ROLLS_1;
            const size_t idx = std::uniform_int_distribution<size_t>(0z, rolls.size() - 1)(get_thread_local_engine());
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


