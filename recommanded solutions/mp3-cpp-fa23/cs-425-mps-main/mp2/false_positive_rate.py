# MP 2
import matplotlib.pyplot as plt
import numpy as np

def plot(drop_rates: list, false_positives_gossip: list, false_positives_gossip_suspicion: list) -> None:
    fig, ax = plt.subplots()
    fig.set_figwidth(15)
    fig.set_figheight(5)

    # plot mean and std. deviation of each trial
    x = np.array(drop_rates)
    false_positives_gossip_np_array = [np.array(y_i) for y_i in false_positives_gossip]
    false_positives_gossip_suspicion_np_array = [np.array(y_i) for y_i in false_positives_gossip_suspicion]

    y_gossip = np.array([y_i.mean() for y_i in false_positives_gossip_np_array])
    e_gossip = np.array([y_i.std() for y_i in false_positives_gossip_np_array])

    y_gossip_suspicion = np.array([y_i.mean() for y_i in false_positives_gossip_suspicion_np_array])
    e_gossip_suspicion = np.array([y_i.std() for y_i in false_positives_gossip_suspicion_np_array])

    gossip_color = 'red'
    gossip_suspicion_color = 'green'
    ax.errorbar(x, y_gossip, yerr=e_gossip, fmt='o', linewidth=2, capsize=6,ecolor=gossip_color, label='Gossip')
    ax.errorbar(x, y_gossip_suspicion, yerr=e_gossip_suspicion, fmt='o', linewidth=2, capsize=6,ecolor=gossip_suspicion_color, label='Gossip With Suspicion')
    ax.legend()

    # add text next to each data point listing data point mean and std. deviation
    for i in range(len(x)):
        ax.annotate(f'{y_gossip[i]:.2f} +/- {e_gossip[i]:.2f}', (x[i], y_gossip[i]), textcoords='offset points',
                 xytext=(2, 5), fontsize='x-small', color=gossip_color)
        ax.annotate(f'{y_gossip_suspicion[i]:.2f} +/- {e_gossip_suspicion[i]:.2f}', (x[i], y_gossip_suspicion[i]), textcoords='offset points',
                 xytext=(2, -10), fontsize='x-small', color=gossip_suspicion_color)
        ax.grid()
    plt.xticks(range(0, 80, 5))

    plt.xlabel('Packet drop rate (%)')
    plt.ylabel('Mean +/- standard deviation of total number of\nfalse positives')
    plt.title('Mean and standard deviation of total number of false positives vs packet drop rate\n')
    plt.show()


drop_rates = [0, 5, 12, 20, 30, 50, 70]
# with gossip (per trial, run program for 300 gossips)
false_positives_gossip = [
        [0, 0, 0, 0, 0, 0, 0], # 0
        [0, 0, 0, 0, 0, 0, 0], # 5
        [0, 0, 0, 0, 0, 0, 0], # 12
        [2, 0, 0, 0, 0, 1, 0], # 20
        [1, 0, 0, 1, 0, 0, 0], # 30
        [2, 3, 0, 2, 1, 0, 2], # 50
        [10, 10, 10, 10, 10, 10, 10], # 70
        ]

# with gossip and suspicion (per trial, run program for 300 gossips)
false_positives_gossip_suspicion = [
        [0, 0, 0, 0, 0, 0, 0], # 0
        [0, 0, 0, 0, 0, 0, 0], # 5
        [0, 0, 0, 0, 0, 0, 0], # 12
        [0, 0, 0, 0, 0, 0, 0], # 20
        [0, 0, 1, 0, 0, 0, 0], # 30
        [0, 0, 0, 0, 0, 1, 0], # 50
        [5, 4, 6, 5, 3, 4, 6], # 70
        ]
plot(drop_rates, false_positives_gossip, false_positives_gossip_suspicion)
