import os
import subprocess
from collections import defaultdict

dataset = "Yelp"


def col_from_flag(flag):
    if flag == "b":
        return "build"
    elif flag == "m":
        return "memory"
    elif flag == "a":
        return "c_access"
    else:
        return "r_access"


def col_from_version(version):
    if version == 1:
        return "Tree"
    elif version == 2:
        return "Nested Hashmap"
    elif version == 3:
        return "Flattened Struct"
    else:
        return "Indexed String"


if __name__ == "__main__":
    for flag in ["b", "m", "a", "r"]:
        with open("data{}_{}.txt".format(flag, dataset), "w") as f:
            # columns
            f.write("{},dataset,type\n".format(col_from_flag(flag)))
            # runs
            for version in [1, 2, 3, 4]:
                # compile and do warmup runs
                subprocess.check_call(["./test.sh", "-{}".format(flag), str(version)])
                # do actual runs and record
                num_runs = 5 if flag != "m" else 1
                for _ in range(num_runs):
                    txt = subprocess.check_output(["./prog"])
                    txt = txt.decode("utf-8")
                    lines = txt.split("\n")
                    if flag == "b":
                        data = lines[1][6:-1]
                    elif flag == "m":
                        data = lines[1][8:-2]
                    else:
                        data = lines[2][6:-1]
                    f.write(
                        "{},{},{}\n".format(data, dataset, col_from_version(version))
                    )
