#!/usr/bin/python3

# python -m venv env && env\bin\activate && python -m pip install scipy

from scipy.fftpack import dct

import numpy as np

if __name__ == "__main__":
    np.set_printoptions(precision=3)

    points = np.array([-1.0, 2.0, 3.0, 6.0, -3.0, -2.0, 0.0, 3.0])

    print(f"Points: {points}")

    dct_res = dct(points, norm="ortho")

    print(f"DCT: {dct_res}")