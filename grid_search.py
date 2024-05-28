#!/usr/bin/env python3
import subprocess
import re
import pandas as pd
import os
from itertools import product

def execute_script(input_, output_):
    cmd = "make clean"
    subprocess.call(cmd, shell=True)
    cmd = "make -j3 %s > %s" % (input_, output_)
    subprocess.call(cmd, shell=True)

def extract_data(in_):
    with open(in_, mode="r") as ifstream:
        file = ifstream.read()
        ss = re.findall("[0-9]+ ms", file)
        build_time = int(ss[3].split(' ')[0])
        run_time = int(ss[-10].split(' ')[0])
        
        ss = re.findall(":= [0-9]+\\.[0-9]+", file)
        recall = float(ss[0].split(' ')[1])

        return {'build_time': build_time, 'run_time': run_time, 'recall': recall}

def extract_exaustive(in_):
    with open(in_, mode="r") as ifstream:
        file = ifstream.read()
        ss = re.findall("[0-9]+ ms", file)
        exaustive = int(ss[-9].split(' ')[0])

        return {'exaustive': exaustive}

def exaustive():
    cum = {}
    for entry in os.listdir('outputs'):
        inc = extract_exaustive(os.path.join('outputs', entry))
        cum = aggregate(cum, inc)
    save_df(cum, "plots/ex.csv")

def aggregate(cum, inc):
    for key in inc:
        if key not in cum:
            cum[key] = []
        cum[key].append(inc[key])
    return cum

def save_df(cum: dict, out: str):
    return pd.DataFrame(cum).to_csv(out, index=False)

def craft_header(data):
    file = open("include/sigmod/custom.hh", "w")
    file.write("#ifndef CUSTOM_HH\n")
    file.write("#define CUSTOM_HH\n")
    for key in data:
        file.write("#define %s %s\n" % (key, data[key]))
    file.write("#endif\n")
    file.close()

def grid_search():
    cum = {}
    LSH_TABLES = [1, 5, 10, 15, 20, 25, 30, 35]
    LSH_FOREST_TRESHOLD = [0, 500, 1000, 2000, 5000, 7000, 10000, 15000]
    values = product(LSH_TABLES, LSH_FOREST_TRESHOLD)
    for (lt, lft) in values:
        craft_header({
            'LSH_TABLES': "%s" % lt,
            'LSH_FOREST_TRESHOLD': "%s"  % lft
        })
        out = 'output-contest-10m-COMB-%s-%s.txt' % (lt, lft)
        execute_script('contest-10m', out)
        inc = extract_data(out)
        inc['LSH_TABLES'] = lt
        inc['LSH_FOREST_TRESHOLD'] = lft
        cum = aggregate(cum, inc)
    save_df(cum, "plots/LSH_COMB.csv")

grid_search()