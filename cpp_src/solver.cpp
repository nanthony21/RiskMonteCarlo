#include <vector>
#include <array>
#include <random>
#include <ranges>
#include <algorithm>
#include <cassert>
#include "solver.hpp"
#include <chrono>
#include <future>
#include <variant>

#ifndef RISK_SINGLE_THREAD
#include <tbb/tbb.h>
#endif

/**
 *
 * @tparam N  The number of dice
 * @return A list of all possible dice rolls for `N` dice. Each roll is sorted so that the highest die comes first.
 */
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

//Possible rolls of a single die
static auto ROLLS_1 = initialize_possible_rolls<1>();
//Possible rolls of 2 dice
static auto ROLLS_2 = initialize_possible_rolls<2>();
//Possible rolls of 3 dice
static auto ROLLS_3 = initialize_possible_rolls<3>();

// Thread-local randomizer allows for running simulations in parallel
std::default_random_engine& get_thread_local_engine() {
    thread_local std::default_random_engine gen(std::random_device {}());
    return gen;
}

// Helper function to reserve `N` elements in a series of std::vectors.
auto reserve_all = [](size_t N, auto&... vecs) {
    (vecs.reserve(N), ...); // fold expression (C++17)
};

template<typename T, typename return_t>
concept SolverC = requires(T a) {
    // Aggregate function should return a `return_t` for the times that attackers won and a `return_t` for the times that defenders won.
    { a.aggregate() } -> std::convertible_to<std::tuple<return_t, return_t>>;

    // Collect function must be thread-safe. Will be called with the remaining attackers and defenders for each simulated battle.
    { a.collect(0, 0) } -> std::convertible_to<void>;
};

/**
 * CRTP base class for a type that calls `solve_attack` in parallel. Derived classes determine how to collect and aggregate the data.
 *
 * @tparam Derived The type of the derived class (used for static polymorphism)
 * @tparam return_t The return type of the `solve_n` function
 */
template<typename Derived, typename return_t>
struct Solver {

    std::tuple<return_t, return_t> solve_n(size_t N, int attackers, int defenders, bool atk_has_leader, bool def_has_leader, bool def_is_capitol) {
        auto iter = [this](int attackers, int defenders, bool atk_has_leader, bool def_has_leader, bool def_is_capitol) {
            auto [a,b] = solve_attack(attackers, defenders, atk_has_leader, def_has_leader, def_is_capitol);
            static_cast<Derived*>(this)->collect(a, b);;
        };
#ifdef RISK_SINGLE_THREAD
        for (size_t i=0; i<N; ++i) {
            iter(attackers, defenders, atk_has_leader, def_has_leader, def_is_capitol);
        }
#else
        tbb::parallel_for(tbb::blocked_range<size_t>(0, N, N / 16), [iter, attackers, defenders, atk_has_leader, def_has_leader, def_is_capitol](tbb::blocked_range<size_t> const& rng) {
            for (auto it = rng.begin(); it!=rng.end(); ++it) {
                iter(attackers, defenders, atk_has_leader, def_has_leader, def_is_capitol);
            }
        });
#endif
        return static_cast<Derived*>(this)->aggregate();
    }
};


struct HistogramSolver: Solver<HistogramSolver, std::vector<size_t>> {

    // Histogram where the value is the frequency count and the number of remaining attackers/defenders is given by the index.
    using hist_t = std::vector<size_t>;

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

    void collect(int a, int d) {
        if (a > 1) {
            ++atk_count[a];
        }
        else
            ++def_count[d];
    }
    std::vector<std::atomic<size_t>> atk_count;
    std::vector<std::atomic<size_t>> def_count;
};

static_assert(SolverC<HistogramSolver, typename HistogramSolver::hist_t>);


std::tuple<std::vector<size_t>, std::vector<size_t>> solve_n_attacks(size_t N, int attackers, int defenders, bool atk_has_leader, bool def_has_leader, bool def_is_capitol) {
    auto [att, def] = HistogramSolver(attackers, defenders).solve_n(N, attackers, defenders, atk_has_leader, def_has_leader, def_is_capitol);
    return std::tuple {att, def};
}

 std::tuple<int, int> solve_attack(int attackers, int defenders, bool atk_has_leader, bool def_has_leader, bool def_is_capitol) {
    while (true) {
        std::variant<std::array<uint8_t, 1>, std::array<uint8_t, 2>, std::array<uint8_t, 3>> att_roll;
        if (attackers >= 4) {
            if (def_is_capitol) { // capitol can only be attacked with 2 dice
                auto rolls = ROLLS_2;
                const size_t idx = std::uniform_int_distribution<size_t>(0z, rolls.size() - 1)(get_thread_local_engine());
                att_roll = rolls[idx];
            }
            else { // default behavior allows attacking with 3 dice
                auto rolls = ROLLS_3;
                const size_t idx = std::uniform_int_distribution<size_t>(0z, rolls.size() - 1)(get_thread_local_engine());
                att_roll = rolls[idx];
            }
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
                defenders
            };
        }
    }
}