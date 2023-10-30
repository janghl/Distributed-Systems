import matplotlib.pyplot as plt
import numpy as np
import random


x = np.array([0, 20, 40, 60, 80, 95]) # pattern frequencies used in experiment
scale = 1 # can be used to change units of y-axis
# below is the latency (in microseconds) for each trial (there were 7 trials per data point x)
y0 = np.array([29353,
26585,
26221,
26185,
27212,
26615,
26539]) / scale
y20 = np.array([66996,
66965,
68765,
67907,
68081,
67157,
67023]) / scale
y40 = np.array([62269,
62445,
61489,
62457,
63029,
62519,
67194]) / scale
y60 = np.array([70219,
68076,
63598,
62812,
63011,
62741,
62943]) / scale
y80 = np.array([80982,
82141,
71986,
71652,
72708,
73436,
71779]) / scale
y100 = np.array([54808,
56891,
55454,
54860,
55063,
56061,
55419]) / scale
# plot mean and std. deviation of each trial
y = np.array([y0.mean(), y20.mean(), y40.mean(), y60.mean(), y80.mean(), y100.mean()])
e = np.array([y0.std(), y20.std(), y40.std(), y60.std(), y80.std(), y100.std()])

fig, ax = plt.subplots()

ax.errorbar(x, y, yerr=e, fmt='o', linewidth=2, capsize=6)

# add text next to each data point listing data point mean and std. deviation
for i in range(len(x)):
    ax.annotate(f'{y[i]:.2f} +/-\n     {e[i]:.2f}', (x[i], y[i]), textcoords='offset points',
                xytext=(2, -5), fontsize='x-small')
ax.grid()
plt.xticks(range(0, 140, 20))

plt.xlabel('Pattern frequency in percent (%)')
plt.ylabel('Mean +/- standard deviation of grep latency\n(microseconds)')
plt.title('Mean and standard deviation of grep latency vs grep pattern with frequency')
plt.show()

