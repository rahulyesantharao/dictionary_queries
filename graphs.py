import numpy as np
import matplotlib.pyplot as plt

dataset = "Yelp"
measurement_type = 3

if dataset == "Twitter":
    data_color = "#00ACEE"
elif dataset == "Yelp":
    data_color = "#c41200"
else:
    print("Invalid dataset")

if measurement_type == 1:
    y_label = "Average Build Time (s)"
    names = ["cJSON", "Hashmap", "Flattened", "Serial"]
elif measurement_type == 2:
    y_label = "Average Memory Usage for Build (GB)"
    names = ["cJSON", "Hashmap", "Flattened", "Serial"]
elif measurement_type == 3:
    y_label = "Average Field Access Time (s)"
    names = ["cJSON", "Hashmap", "Flattened C", "Flattened R", "Serial"]
else:
    print("Invalid Measurement Type!")

if dataset == "Twitter":
    # Twitter
    if measurement_type == 1:
        # build times (s)
        data = [
            [60.2339, 49.3238, 53.9777, 60.7755, 60.777],
            [60.0574, 57.3929, 49.8694, 53.507, 57.8562],
            [64.474, 51.1145, 50.3161, 54.0222, 58.3576],
            [57.9821, 61.146, 53.7519, 62.6825, 64.0363],
        ]
    elif measurement_type == 2:
        # memory usage (gb)
        data = [[2.21071], [3.67976], [2.58798], [3.8123]]
    elif measurement_type == 3:
        # access times (s)
        data = [
            [0.0705415, 0.0703834, 0.0711797, 0.0706068, 0.0706189],
            [0.139616, 0.13858, 0.137659, 0.141176, 0.139595],
            [0.00439655, 0.00445112, 0.00438867, 0.00438679, 0.00440079],
            [0.0649305, 0.0654571, 0.065226, 0.0651703, 0.0704154],
            [0.0548597, 0.0552568, 0.0549764, 0.0549033, 0.0560956],
        ]
else:
    if measurement_type == 1:
        # build times (s)
        data = [
            [2.41078, 2.43933, 2.4254, 2.41501, 2.40656],
            [2.5934, 2.59184, 2.60951, 2.60139, 2.59061],
            [1.72849, 1.72708, 1.72652, 1.7279, 1.73144],
            [2.46973, 2.46706, 2.45018, 2.45214, 2.45356],
        ]
    elif measurement_type == 2:
        # memory usage (gb)
        data = [[0.0905232], [0.838853], [0.309254], [1.07112]]
    elif measurement_type == 3:
        # access times (s)
        data = [
            [0.140835, 0.141117, 0.139427, 0.141094, 0.140551],
            [0.179848, 0.180007, 0.178557, 0.180915, 0.181072],
            [0.0160975, 0.0160962, 0.0161177, 0.0168331, 0.015921],
            [0.210535, 0.21155, 0.21284, 0.210538, 0.211017],
            [0.191591, 0.193863, 0.194273, 0.192048, 0.191535],
        ]


if __name__ == "__main__":
    avg = [np.mean(d) for d in data]
    std_dev = [np.std(d) for d in data]
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
