import argparse
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
import risk_solver

logger = logging.getLogger(__name__)


class AttackResult(NamedTuple):
    remaining_attacks: int
    remaining_defenders: int
    attacker_wins: bool

class AttackParams(NamedTuple):
    attackers: int
    atk_has_leader: bool
    defenders: int
    def_has_leader: bool


@dataclass
class ExperimentResult:
    atk_hist: list[int]
    def_hist: list[int]


def run_experiment(atk_params: AttackParams, num_trials: int) -> ExperimentResult:
    atk_win_hist, def_win_hist = risk_solver.solve_n_attacks(num_trials, atk_params.attackers, atk_params.defenders, atk_params.atk_has_leader, atk_params.def_has_leader)
    return ExperimentResult(atk_win_hist, def_win_hist)


def plot_result(result: ExperimentResult, params: AttackParams) -> DockablePlotWindow:
    df = (pd.concat([pd.DataFrame(pd.Series(result.def_hist)), pd.DataFrame(pd.Series(result.atk_hist))], keys=("def", "atk"), names=['side'])
           .reset_index()
           .reset_index(drop=True)
          .rename({0: 'count', 'level_1': 'remain'}, axis=1))
    df[['count', 'remain']] = df[['count', 'remain']].astype(float)
    df['percent'] = df['count'] / df['count'].sum() * 100
    plots = DockablePlotWindow(str(params))
    fig, ax = plots.subplots("combined", 'left')
    sns.histplot(data=df, weights='percent', hue='side',x='remain', ax=ax, binwidth=1, stat='percent')
    for side, g in df.groupby("side"):
        fig, ax = plots.subplots(side, "right")
        sns.histplot(data=g, weights='percent',x='remain', ax=ax, binwidth=1, stat='percent')
        fig.suptitle(f"{side} : {g['percent'].sum() / df['percent'].sum() * 100: .2f}% to win")

    # df = pd.concat([pd.Series(result.atk_hist), pd.Series(-1 * np.array(result.def_wins))], axis=0)
    df['remain'] = df.apply(lambda row: -row['remain'] if row['side'] == 'def' else row['remain'], axis=1)
    df = df.sort_values('remain')
    df = pd.DataFrame(df)
    fig, ax = plots.subplots("stacked hist")
    sns.histplot(data=df, weights="percent", x='remain',  hue='side', binwidth=1, ax=ax, stat='percent')
    cum = np.cumsum(df['count'])
    cum /= cum.max()
    thresh = [df['remain'].iloc[np.argmin((cum - p).abs())] for p in [.25, .5, .75]]
    ax.vlines(x=thresh, ymin=0, ymax=df['percent'].max(), color=['k', 'k', 'k'], linestyles=['--', '-', '--'])
    return plots

def cli_main():
    parser = argparse.ArgumentParser("risk-sim", description="Simple Monte Carlo simulator for predicting outcomes of a battle in the game of Risk")
    parser.add_argument('num_attackers', type=int)
    parser.add_argument('num_defenders', type=int)
    parser.add_argument('-n', '--num-trials', dest='num_trials', type=int, default=100000)
    parser.add_argument('-a', '--atk-has-leader', dest='atk_has_leader', action='store_true')
    parser.add_argument('-d', '--def-has-leader', dest='def_has_leader', action='store_true')

    args = parser.parse_args()
    
    param = AttackParams(args.num_attackers, args.atk_has_leader, args.num_defenders, args.def_has_leader)
    stime = time.time()
    result = run_experiment(param, args.num_trials)
    print(f"experiment finished in {time.time() - stime} seconds")
    app = QApplication([])
    plots = plot_result(result, param)
    app.exec()

if __name__ == '__main__':
    cli_main()
    # import sys
    # logging.basicConfig(stream=sys.stdout, level=logging.INFO)
    # # parms = AttackParams(50, False, 55, False)
    # app = QApplication([])
    # w = []
    # for parm in [
    #     AttackParams(15, False, 21, False),
    #     # AttackParams(30, True, 40, False),
    #     # AttackParams(50, True, 60, True),
    #     # AttackParams(50, True, 60, False),
    # ]:
    #     print("beginning experiment")
    #     stime = time.time()
    #     result = run_experiment(parm, 100000, None)
    #     print(f"experiment finished in {time.time() - stime} s")
    #     plots = plot_result(result, parm)
    #     w.append(plots)
    # app.exec()
    # # plt.show(block=True)
    # e = 1

