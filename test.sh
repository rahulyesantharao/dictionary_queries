#!/bin/bash
testing=0
mode=0

# validate parameters
if [ $# -ne 2 ]; then
  echo "usage test.sh -[b|m|a] <rep type (1-4)>"
  exit 1
fi
if [ "$1" != "-b" ] && [ "$1" != "-m" ] && [ "$1" != "-a" ]; then
  echo -e "invalid flag: $1\nusage: test.sh [-b|m|a] <rep type (1-4)>\nb=build time, m=mem usage, a=access time"
  exit 1
fi
if [ $2 -ne 1 ] && [ $2 -ne 2 ] && [ $2 -ne 3 ] && [ $2 -ne 4 ]; then
  echo "usage: 1 = cJSON, 2 = Hashmap, 3 = Flattened, 4 = Serial"
  exit 1
fi

# parse parameters
if [ "$1" == "-b" ]; then
  mode=1
elif [ "$1" == "-m" ]; then
  mode=2
else
  mode=3
fi
testing=$2

# compile correct version of code
if [ $mode -eq 1 ]; then
  echo "Timing Test"
  g++ -std=c++17 -DTESTING=$testing -DTIME -DTIMEBUILD cJSON/cJSON.c part1.cpp -o prog
elif [ $mode -eq 2 ]; then
  echo "Memory Test"
  g++ -std=c++17 -DTESTING=$testing -DMEMORY cJSON/cJSON.c part1.cpp -o prog
else
  echo "Access Test"
  g++ -std=c++17 -DTESTING=$testing -DTIME -DTIMEACCESS cJSON/cJSON.c part1.cpp -o prog
fi
echo "Compiled"

# run the test (TODO: make it run multiple times)
./prog
