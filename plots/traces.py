import pandas as pd
import matplotlib.pyplot as plt
import os

def collect_traces(dir):
    traces = {}
    for entry in os.listdir(dir):
        id, _ = entry.split('.')
        id = int(id)
        traces[id] = os.path.join(dir, entry)
    return traces

def plot_trace(id, path):
    df = pd.read_csv(path)
    plt.plot(df.Rank, df.Score, marker='o', label=('%s' % id))

def plot_traces(traces):
    for id in sorted(traces.keys()):
        plot_trace(id, traces[id])

def produce_traces_plot(dir):
    plt.figure(figsize=(30, 30))
    plt.xlabel('Rank')
    plt.ylabel('Score')
    plot_traces(collect_traces(dir))
    plt.legend()
    plt.savefig('%s.png' % dir)

def produce_all_traces_plots():
    for entry in os.listdir('traces'):
        produce_traces_plot(os.path.join('traces', entry))

produce_all_traces_plots()
