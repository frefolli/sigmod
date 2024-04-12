import pandas as pd

df = pd.read_csv('out.csv')
print(df.sort_values(by='MSE'))
