#!/bin/bash
testing=0
mode=0

# validate parameters
if [ $# -ne 2 ]; then
  echo "usage test.sh -[b|m|a] <rep type (1-4)>"
  exit 1
fi
if [ "$1" != "-b" ] && [ "$1" != "-m" ] && [ "$1" != "-a" ] && [ "$1" != "-r" ]; then
  echo -e "invalid flag: $1\nusage: test.sh [-b|m|a|r] <rep type (1-4)>\nb=build time, m=mem usage, a=access time"
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
elif [ "$1" == "-a" ]; then
  mode=3
else
  mode=4
fi
testing=$2

# compile correct version of code
if [ $mode -eq 1 ]; then
  echo "Timing Test"
  g++ -std=c++17 -O3 -DTESTING=$testing -DTIME -DTIMEBUILD -DTWITTER cJSON/cJSON.c part1.cpp -o prog
elif [ $mode -eq 2 ]; then
  echo "Memory Test"
  g++ -std=c++17 -O3 -DTESTING=$testing -DMEMORY -DTWITTER cJSON/cJSON.c part1.cpp -o prog
elif [ $mode -eq 3 ]; then
  echo "Compile-Time Access Test"
  g++ -std=c++17 -O3 -DTESTING=$testing -DTIME -DTIMEACCESSCOMPILETIME -DTWITTER cJSON/cJSON.c part1.cpp -o prog
else
  echo "Runtime Access Test"
  g++ -std=c++17 -O3 -DTESTING=$testing -DTIME -DTIMEACCESSRUNTIME -DTWITTER cJSON/cJSON.c part1.cpp -o prog
fi
echo "Compiled"

# run the test mutiple times to fix any variability
./prog
