import copy
import itertools
import random
import time
from dataclasses import dataclass
import pandas as pd
import seaborn as sns
import logging
import numpy as np
from PyQt6.QtWidgets import QApplication
from mpl_qt_viz.visualizers import DockablePlotWindow
from typing import NamedTuple
import risk_solver_ext

logger = logging.getLogger(__name__)

# All possible rolls of 3 dice sorted so the highest dice come first
_ROLLS_1 = tuple(tuple(reversed(sorted(i))) for i in itertools.product(range(1,7), repeat=1))
_ROLLS_2 = tuple(tuple(reversed(sorted(i))) for i in itertools.product(range(1,7), repeat=2))
_ROLLS_3 = tuple(tuple(reversed(sorted(i))) for i in itertools.product(range(1,7), repeat=3))


class AttackResult(NamedTuple):
    remaining_attacks: int
    remaining_defenders: int
    attacker_wins: bool

class AttackParams(NamedTuple):
    attackers: int
    atk_has_leader: bool
    defenders: int
    def_has_leader: bool


# def _solve_attack(atk_params: AttackParams):
#     attackers = atk_params.attackers
#     defenders = atk_params.defenders
#     while True:
#         if attackers >= 4:
#             rolls = _ROLLS_3
#             idx = np.random.randint(len(rolls))
#             att_roll = rolls[idx]
#         elif attackers == 3:
#             rolls = _ROLLS_2
#             idx = np.random.randint(len(rolls))
#             att_roll = rolls[idx]
#         else:
#             assert attackers == 2
#             rolls = _ROLLS_1
#             idx = np.random.randint(len(rolls))
#             att_roll = rolls[idx]
#         if atk_params.atk_has_leader:
#             att_roll = (att_roll[0] + 1,) + att_roll[1:]
#         if defenders >= 2:
#             rolls = _ROLLS_2
#             idx = np.random.randint(len(rolls))
#             def_roll = rolls[idx]
#         else:
#             assert defenders == 1
#             rolls = _ROLLS_1
#             idx = np.random.randint(len(rolls))
#             def_roll = rolls[idx]
#         if atk_params.def_has_leader:
#             def_roll = (def_roll[0] + 1,) + def_roll[1:]
#         # Process rolls
#         atk_lose = 0
#         def_lose = 0
#         for i in range(min(len(att_roll), len(def_roll))):
#             if att_roll[i] > def_roll[i]:
#                 def_lose += 1
#             else:
#                 atk_lose += 1
#         defenders -= def_lose
#         attackers -= atk_lose
#         if defenders <= 0 or attackers <= 1:
#             assert defenders >= 0
#             assert attackers >= 1
#             return AttackResult(
#                 attackers,
#                 defenders,
#                 defenders == 0
#             )

# def solve_attack(atk_params: AttackParams):
#     _atk_params = copy.deepcopy(atk_params)
#     return _solve_attack(_atk_params)

@dataclass
class ExperimentResult:
    atk_wins: list[int]
    def_wins: list[int]


def run_experiment(atk_params: AttackParams, num_trials: int, seed: int | None) -> ExperimentResult:
    # random.seed(seed)
    atk_win_with, def_win_with = risk_solver_ext.solve_n_attacks(num_trials, atk_params.attackers, atk_params.defenders, atk_params.atk_has_leader, atk_params.def_has_leader)
    return ExperimentResult(atk_win_with, def_win_with)


def plot_result(result: ExperimentResult) -> DockablePlotWindow:
    df = (pd.concat([pd.Series(result.def_wins), pd.Series(result.atk_wins)], keys=("def", "atk"), names=['side'])
           .reset_index(level='side')
           .reset_index(drop=True)
          .rename({0: 'remaining'}, axis=1))
    plots = DockablePlotWindow()
    fig, ax = plots.subplots("combined", 'left')
    sns.histplot(data=df, x='remaining', hue='side', discrete=True, stat='percent')
    fig, ax = plots.subplots("box", "left")
    sns.boxplot(data=df, x='side', y='remaining')
    for side, g in df.groupby("side"):
        fig, ax = plots.subplots(side, "right")
        sns.histplot(data=g, x='remaining', discrete=True, stat='percent')
        fig.suptitle(f"{side} : {len(g) / len(df) * 100: .2f}% to win")

    df = pd.concat([pd.Series(result.atk_wins), pd.Series(-1 * np.array(result.def_wins))], axis=0)
    df.rename("Remaining Atk", inplace=True)
    df = pd.DataFrame(df)
    fig, ax = plots.subplots("stacked hist")
    sns.histplot(data=df, x="Remaining Atk", discrete=True, stat='percent')
    fig, ax = plots.subplots("stacked box")
    sns.boxplot(data=df, y="Remaining Atk")
    return plots

if __name__ == '__main__':
    import sys
    logging.basicConfig(stream=sys.stdout, level=logging.INFO)
    # parms = AttackParams(50, False, 55, False)
    app = QApplication([])
    w = []
    for parm in [
        AttackParams(30, False, 1, False),
        AttackParams(30, True, 1, True),
        # AttackParams(50, True, 60, True),
        # AttackParams(50, True, 60, False),
    ]:
        print("beginning experiment")
        stime = time.time()
        result = run_experiment(parm, 100000, None)
        print(f"experiment finished in {time.time() - stime} s")
        plots = plot_result(result)
        w.append(plots)
    app.exec()
    # plt.show(block=True)
    e = 1

