#include <vector>
#include <array>
#include <ranges>
#include <pybind11/pybind11.h>

template<int N>
std::vector<std::array<uint8_t, N>> initialize_possible_rolls() {
    std::vector<std::array<uint8_t, N>> out;
    constexpr uint8_t SIDES = 6;
    std::array<uint8_t, N> this_combo;

    auto gen_rolls = [&out, &this_combo](this auto&& self, uint8_t dice_idx) {
        for (uint8_t dice_roll=1; dice_roll<=SIDES; dice_roll++) {
            this_combo[dice_idx] = dice_roll;
            if (dice_idx < N) {
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
        std::ranges::sort(roll_combo, std::less());
    }
}

static auto ROLLS_1 = initialize_possible_rolls<1>();
static auto ROLLS_2 = initialize_possible_rolls<2>();
static auto ROLLS_3 = initialize_possible_rolls<3>();

void solve_attack(int attackers, int defenders, bool atk_has_leader, bool def_has_leader) {
    while (true) {
        if (attackers >= 4) {
            auto rolls = ROLLS_3;
            idx = np.random.randint(len(rolls))
            att_roll = rolls[idx]
        }
        else if (attackers == 3) {
            auto rolls = ROLLS_2;
            idx = np.random.randint(len(rolls))
            att_roll = rolls[idx]
        }
        else {
            assert(attackers == 2);
            auto rolls = ROLLS_1
            idx = np.random.randint(len(rolls))
            att_roll = rolls[idx]
        }

        if (atk_has_leader)
            ++(att_roll[0]);

        if (defenders >= 2) {
            auto rolls = ROLLS_2;
            idx = np.random.randint(len(rolls));
            def_roll = rolls[idx];
        }
        else {
            assert(defenders == 1);
            auto rolls = ROLLS_1;
            idx = np.random.randint(len(rolls));
            def_roll = rolls[idx];
        }
        if def_has_leader:
            def_roll = (def_roll[0] + 1,) + def_roll[1:]
        // Process rolls
        int atk_lose = 0;
        int def_lose = 0;
        for (size_t i : std::views::iota(std::min(len(att_roll), len(def_roll)))):
            if (att_roll[i] > def_roll[i]) {
                ++def_lose;
            }
            else {
                ++atk_lose;
            }
        defenders -= def_lose;
        attackers -= atk_lose;
        if ((defenders <= 0) or (attackers <= 1)):
            assert(defenders >= 0);
            assert(attackers >= 1);
            return AttackResult(
                attackers,
                defenders,
                defenders == 0
            )
    }
}

PYBIND11_MODULE(risk_solver_ext, m) {}
