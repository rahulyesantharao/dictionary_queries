import numpy as np
import matplotlib.pyplot as plt

dataset = "Twitter"
if dataset == "Twitter":
    data_color = "#00ACEE"
elif dataset == "Yelp":
    data_color = "#c41200"
else:
    print("Invalid dataset")

measurement_type = 1

if measurement_type == 1:
    y_label = "Average Build Time (s)"
elif measurement_type == 2:
    y_label = "Average Memory Usage for Build (GB)"
elif measurement_type == 3:
    y_label = "Average Field Access Time (s)"
else:
    print("Invalid Measurement Type!")

data = [
    [0.12, 0.13, 0.14, 0.11, 0.15],  # cJSON
    [0.10, 0.09, 0.08, 0.11, 0.10],  # Hashmap
    [0.23, 0.23, 0.25, 0.24, 0.21],  # Flattened
    [0.16, 0.17, 0.18, 0.17, 0.16],  # Serial
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
