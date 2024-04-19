#!/usr/bin/env python3
import subprocess
import re

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
        recall = ss[0]

        return {'build_time': build_time, 'run_time': run_time, 'recall': recall}

def aggregate(cum, inc):
    for key in inc:
        cum[key].append(inc[key])
    return cum

def save_df(cum: dict, out: str):
    return pd.DataFrame(cum).to_csv(out, Index=False)

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
    for dx in [0.25, 0.33, 0.5, 0.66, 0.75, 1,
               1.25, 1.33, 1.5, 1.66, 1.75, 2]:
        craft_header({
            'LSH_TABLES': "(uint32_t) ((float) k_nearest_neighbors * %s)" % dx
        })
        out = 'output-10m-htn-%s.txt' % dx
        execute_script('contest-10m', out)
        inc = extract_data(out)
        inc['HTN'] = dx
        cum = aggregate(cum, inc)
    save_df(cum, "plots/htn.csv")

grid_search()
