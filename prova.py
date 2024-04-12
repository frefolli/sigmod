import struct
import numpy as np
import pandas as pd
import time

"""
double const y = __builtin_bit_cast(double, MAGIC_NUMBER_A - (__builtin_bit_cast(uint64_t, number) >> 1));
return 1 / (y * C * (D - number * y * y))

double const y = __builtin_bit_cast(double, MAGIC_NUMBER_A - (__builtin_bit_cast(uint64_t, number) >> 1));
return 1 / (y * (A - (number * B * y * y))); y * y));

#define MAGIC_NUMBER_A 0x5FE6EB50C7B537A9
#define MAGIC_NUMBER_B 0x5F1FFFF9
#define C 0.703952253f
#define D 2.38924456f
#define A 1.5f
#define B 0.5f
"""

MAGIC_NUMBER = 0x5FE6EB50C7B537A9
A = 0.7221722172217222
B = 2.3894389438943895
C = 1.5
D = 0.5

def l2d(l):
    return struct.unpack('@d', struct.pack('@Q', l))[0]

def d2l(d):
    return struct.unpack('@Q', struct.pack('@d', d))[0]

def quacke3(x):
    y = l2d(MAGIC_NUMBER - d2l(x)//2)
    return 1 / (y * A * (B - x * y * y))

def quacke2(x):
    y = l2d(MAGIC_NUMBER - d2l(x)//2)
    return 1 / (y * (C - (B * x * y * y)))

def squacke3(Xs):
    return np.array([quacke3(x) for x in Xs])

def squacke2(Xs):
    return np.array([quacke2(x) for x in Xs])

def mse(Ys, Zs):
    return np.sqrt(np.sum([pow(y - z, 2) for (y, z) in zip(Ys, Zs)]))

X = np.linspace(0, 3000, num=1000000)
Y = np.sqrt(X)

print("A := ", A)
print("B := ", B)
print("C := ", C)
print("D := ", D)
now = time.time()
print("X.shape := ", X.shape)
print("MSE_AB := ", mse(squacke3(X), Y))
end = time.time()
print("ETA (s) := ", end - now)
now = time.time()
print("MSE_CD := ", mse(squacke2(X), Y))
end = time.time()
print("ETA (s) := ", end - now)
