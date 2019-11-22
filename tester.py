import os
import subprocess

if __name__ == "__main__":
    with open("time.txt", "w") as f:
        for flag in ["b", "m", "a"]:
            for version in [1, 2, 3, 4, 5]:
                f.write("{} {}:\n".format(flag, version))
                if version == 5 and flag != "a":
                    continue
                # compile and do warmup runs
                subprocess.check_call(["./test.sh", "-{}".format(flag), str(version)])
                # do actual runs and record
                num_runs = 3 if flag != "m" else 1
                f.write("[")
                for _ in range(num_runs):
                    txt = subprocess.check_output(["./prog"])
                    txt = txt.decode("utf-8")
                    # print(txt)
                    lines = txt.split("\n")
                    # print(lines)
                    if flag == "b":
                        data = lines[1][6:-1]
                    elif flag == "m":
                        data = lines[1][8:-2]
                    else:
                        data = lines[2][6:-1]
                    f.write("{},".format(data))
                f.write("]\n")
