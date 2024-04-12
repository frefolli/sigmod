import struct
import numpy as np
from scipy.optimize import curve_fit

"""
double const y = __builtin_bit_cast(double, MAGIC_NUMBER_A - (__builtin_bit_cast(uint64_t, number) >> 1));
return 1 / (y * C * (D - number * y * y));

#define MAGIC_NUMBER_A 0x5FE6EB50C7B537A9
#define MAGIC_NUMBER_B 0x5F1FFFF9
#define C 0.703952253f
#define D 2.38924456f
"""

def quacke3(x, C, D):
    print(x.shape)
    y = (-(x.view(np.uint32) / 2) + 0x5F1FFFF9).view(np.float32)
    return 1 / (y * C * (D - x * y * y))

# Generate some sample data
x_data = list(np.linspace(0, 300, num=200000, dtype=np.float32))
y_data = np.sqrt(x_data)

C0 = 0.703952253
D0 = 2.38924456

# Fit the data to the logarithm function
popt, pcov = curve_fit(quacke3, x_data, y_data, p0=(C0, D0))
print(y_data[434], quacke3(x_data[434], *popt))
print(popt)
