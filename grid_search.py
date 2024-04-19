#!/usr/bin/env python3
import subprocess
import re
import pandas as pd

def execute_script(input_, output_):
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
    for dx in range(1, 100, 5):
        craft_header({
            'LSH_SPREAD': "%s" % dx
        })
        out = 'output-10m-spd-%s.txt' % dx
        execute_script('contest-10m', out)
        inc = extract_data(out)
        inc['SPD'] = dx
        cum = aggregate(cum, inc)
    save_df(cum, "plots/spd.csv")

grid_search()
