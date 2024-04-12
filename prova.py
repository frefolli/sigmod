import struct
import numpy as np
from scipy.optimize import curve_fit

"""
double const y = __builtin_bit_cast(double, MAGIC_NUMBER_A - (__builtin_bit_cast(uint64_t, number) >> 1));
return 1 / (y * C * (D - number * y * y));

#define MAGIC_NUMBER_A 0x5FE6EB50C7B537A9
#define C 0.703952253f
#define D 2.38924456f
"""

def double_to_ulong(double):
    return struct.unpack('@Q', struct.pack('@d', f))[0]

def ulong_to_double(double):
    return struct.unpack('@d', struct.pack('@Q', f))[0]

def quacke3(x, C, D):
    y = ulong_to_double(0x5FE6EB50C7B537A9 - (double_to_ulong(x) // 2))
    return 1 / (y * C * (D - x * y * y))

# Generate some sample data
x_data = ["x"]
y_data = ["sqrt(x)"]

# Fit the data to the logarithm function
popt, pcov = curve_fit(quacke3, x_data, y_data)
print(popt)
