import numpy as np
import matplotlib.pyplot as plt

data = [
    [0.12, 0.13, 0.14, 0.11, 0.15],
    [0.10, 0.09, 0.08, 0.11, 0.10],
    [0.23, 0.23, 0.25, 0.24, 0.21],
    [0.16, 0.17, 0.18, 0.17, 0.16],
]

if __name__ == "__main__":
    avg = [np.mean(d) for d in data]
    std_dev = [np.std(d) for d in data]
    plt.errorbar(range(4), avg, yerr=std_dev, fmt=".k")
    plt.show()
