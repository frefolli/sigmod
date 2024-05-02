import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import os

def partition(Xs):
    total = sum(Xs)
    return [ sum(Xs[:i+1]) * 100 / total for i in range(len(Xs)) ]

def mean(Xs):
    return sum(Xs) / len(Xs)

def get_lsh_list():
    for entry in os.listdir('lsh_dump'):
        entry_p = os.path.join('lsh_dump', entry)
        if os.path.isdir(entry_p):
            yield entry_p

def get_hashtables_list(lsh_dir):
    for entry in os.listdir(lsh_dir):
        entry_p = os.path.join(lsh_dir, entry)
        if os.path.isfile(entry_p):
            yield entry_p

def load_hashtables_of_lsh(lsh_dir):
    hashtables = {}
    for ht_path in get_hashtables_list(lsh_dir):
        data = pd.read_csv(ht_path)
        key, _ = os.path.basename(ht_path).split('.')
        key = int(key)
        hashtables[key] = data
    return hashtables

def plot_hashtables(hashtables: dict, output: str):
    plt.figure(figsize=(30, 30))
    for key in hashtables:
        df = hashtables[key]
        plt.plot(range(len(df)), df.Count, marker='o', label=('H%s ([%s, %s]), (min = %s), (max = %s), (mean = %s)' % (key, min(df.ID), max(df.ID), min(df.Count), max(df.Count), mean(df.Count))))
    plt.xlabel('#')
    plt.ylabel('Count')
    plt.legend()
    plt.savefig(output)
    plt.close()

def plot_distribution(hashtables: dict, output: str):
    plt.figure(figsize=(30, 30))
    for key in hashtables:
        df = hashtables[key]
        plt.plot(range(len(df)), sorted(df.Count), marker='o', label=('sH%s ([%s, %s]), (min = %s), (max = %s), (mean = %s)' % (key, min(df.ID), max(df.ID), min(df.Count), max(df.Count), mean(df.Count))))
    plt.xlabel('#')
    plt.ylabel('Sorted Count')
    plt.legend()
    plt.savefig(output)
    plt.close()

def plot_partition(hashtables: dict, output: str):
    plt.figure(figsize=(30, 30))
    for key in hashtables:
        df = hashtables[key]
        plt.plot(range(len(df)), partition(df.Count), marker='o', label=('sH%s ([%s, %s]), (min = %s), (max = %s), (mean = %s)' % (key, min(df.ID), max(df.ID), min(df.Count), max(df.Count), mean(df.Count))))
    plt.xlabel('#')
    plt.ylabel('Partition (perc %)')
    plt.legend()
    plt.savefig(output)
    plt.close()

def plot_lsh_forest():
    for lsh_dir in ['lsh_dump/general']:#get_lsh_list():
        print("Processing LSH := %s" % lsh_dir)
        lsh_key = os.path.basename(lsh_dir)
        hashtables = load_hashtables_of_lsh(lsh_dir)
        if len(hashtables) != 0:
            plot_hashtables(hashtables, 'hts_%s.png' % lsh_key)
            plot_distribution(hashtables, 'dst_%s.png' % lsh_key)
            plot_partition(hashtables, 'ptr_%s.png' % lsh_key)
            data = pd.read_csv(lsh_dir + '.csv')

plot_lsh_forest()
