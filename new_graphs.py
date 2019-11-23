import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd

case = 3
data_set = "T"
hue_color = "#c41200" if data_set == "Y" else "#00ACEE"

if case == 1:
    # build time
    filename = "datab.csv"
    col = "build"
    col_to_graph = "mb_per_sec"
    data_label = "build speed (MB/s)"
elif case == 2:
    # runtime
    filename = "final_data/datar{}.csv".format(data_set)
    image_name = "rAccess{}".format(data_set)
    col = "r_access"
    col_to_graph = "r_access"
    data_label = "Access Time (s)"
elif case == 3:
    # compile time
    filename = "final_data/dataa{}.csv".format(data_set)
    image_name = "aAccess{}".format(data_set)
    col = "c_access"
    col_to_graph = "c_access"
    data_label = "Access Time (s)"
else:
    # compile time
    filename = "datam.csv"
    col = "memory"
    col_to_graph = "memory"
    data_label = "Memory Usage (GB)"

# open csv
df = pd.read_csv(filename)

# add speed column if needed
yelp_size = 239825190 / 1000000
twitter_size = 7685509092 / 1000000
if col == "build":
    df.loc[df["dataset"] == "Twitter", col_to_graph] = (
        twitter_size / df[df["dataset"] == "Twitter"][col]
    )
    df.loc[df["dataset"] == "Yelp", col_to_graph] = (
        yelp_size / df[df["dataset"] == "Yelp"][col]
    )


# just some cool info
df.groupby(["dataset", "type"])[col].describe()

sns.set_style("whitegrid")
pal = [sns.color_palette("Blues_d", 9)[2], sns.color_palette("Blues_d", 9)[7]]
sns.set_palette(pal)
plt.figure(figsize=(10, 5))
ax = sns.barplot(
    x="type",
    y=col_to_graph,
    hue="dataset",
    data=df,
    order=["Nested Hashmap", "Tree", "Indexed String", "Flattened Struct"],
    palette=[hue_color],
)
plt.xlabel("")
plt.ylabel(data_label, fontsize=16)
plt.xticks(fontsize=14)
plt.yticks(fontsize=14)
sns.despine()

handles, labels = ax.get_legend_handles_labels()
ax.legend(
    handles=handles,
    labels=[l.capitalize() for l in labels],
    fontsize=16,
    loc="upper right",
)
# plt.ylim(0, 10)
plt.tight_layout()
plt.savefig("{}.pdf".format(image_name), dpi=120, transparent=True)
