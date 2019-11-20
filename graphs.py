import numpy as np
import matplotlib.pyplot as plt

dataset = "Twitter"
measurement_type = 3

if dataset == "Twitter":
    data_color = "#00ACEE"
elif dataset == "Yelp":
    data_color = "#c41200"
else:
    print("Invalid dataset")

if measurement_type == 1:
    y_label = "Average Build Time (s)"
elif measurement_type == 2:
    y_label = "Average Memory Usage for Build (GB)"
elif measurement_type == 3:
    y_label = "Average Field Access Time (s)"
else:
    print("Invalid Measurement Type!")

if dataset == "Twitter":
    # Twitter
    if measurement_type == 1:
        # build times (s)
        data = [
            [0.327528, 0.321072, 0.329142, 0.322101, 0.32107],  # cJSON
            [0.346682, 0.347806, 0.336928, 0.334666, 0.335561],  # Hashmap
            [0.337194, 0.324981, 0.332409, 0.328471, 0.324299],  # Flattened
            [0.362119, 0.356035, 0.352393, 0.350262, 0.350938],  # Serial
        ]
    elif measurement_type == 2:
        # memory usage (gb)
        data = [
            [2.30855],  # cJSON
            [2.32742],  # Hashmap
            [2.31359],  # Flattened
            [2.33012],  # Serial
        ]
    elif measurement_type == 3:
        # access times (s)
        data = [
            [0.000789718, 0.000730146, 0.000703539, 0.000724039, 0.000874605],  # cJSON
            [0.00119009, 0.00115043, 0.00115133, 0.00116425, 0.0013829],  # Hashmap
            [8.1029e-05, 6.9883e-05, 7.2022e-05, 6.6906e-05, 6.8536e-05],  # Flattened
            [0.000412709, 0.000566911, 0.000538181, 0.000453178, 0.000469591],  # Serial
        ]

if __name__ == "__main__":
    avg = [np.mean(d) for d in data]
    std_dev = [np.std(d) for d in data]
    names = ["cJSON", "Hashmap", "Flattened", "Serial"]
    x_pos = np.arange(len(names))

    # build chart
    fig, ax = plt.subplots()
    ax.bar(
        x_pos,
        avg,
        yerr=std_dev,
        align="center",
        alpha=1.0,
        ecolor="black",
        capsize=3,
        width=1.0,
        color=data_color,
        edgecolor="black",
    )
    ax.set_ylabel(y_label)
    ax.set_xticks(x_pos)
    ax.set_xticklabels(names)
    ax.set_title(f"Query on {dataset} Data")
    # ax.yaxis.grid(True)

    # show (and save)
    plt.tight_layout()
    # plt.savefig(f'graph_{dataset}_{measurement_type}.png')
    plt.show()
