import seaborn as sns
import matplotlib.pyplot as plt
import pandas as pd
import os
import numpy as np

def loat_from_file(path : str) -> pd.DataFrame:
    df = pd.read_csv(path)
    return df

def plot_df(data : pd.DataFrame, metric : str):
    df = data.pivot(index='LSH_TABLES', columns='LSH_FOREST_TRESHOLD', values=metric)

    fig, ax = plt.subplots()
    im = ax.imshow(df)

    # Show all ticks and label them with the respective list entries
    ax.set_xticks(np.arange(len(data['LSH_TABLES'])), labels=data['LSH_TABLES'])
    ax.set_yticks(np.arange(len(data['LSH_FOREST_TRESHOLD'])), labels=data['LSH_FOREST_TRESHOLD'])

    ax.set_title(metric)
    fig.tight_layout()
    plt.savefig(metric)

plot_df(loat_from_file('outputs/LSH_COMB.csv'), 'recall')
plot_df(loat_from_file('outputs/LSH_COMB.csv'), 'run_time')
plot_df(loat_from_file('outputs/LSH_COMB.csv'), 'build_time')