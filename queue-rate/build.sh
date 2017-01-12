#!/bin/sh

if [ -f ~/intel_compilervars.sh ]; then
    . ~/intel_compilervars.sh intel64
fi
if [ -f /opt/rh/devtoolset-3/enable ]; then
    . /opt/rh/devtoolset-3/enable
fi

if [ "$1" == "" ]; then
    producers=1
else
    producers=$1
fi

if [ "$2" == "" ]; then
    consumers=1
else
    consumers=$2
fi

rm -f mpsc2 mpsc3
g++  -g -O3 -std=c++11 -I/home/cjarcher/code/install/gnu/hwloc/include -I. -I./include -DN_CONSUMERS=${consumers} -DN_PRODUCERS=${producers} src/printme.c src/mpsc2.cc -o mpsc2 -lpthread -L/home/cjarcher/code/install/gnu/hwloc/lib -Wl,-rpath,/home/cjarcher/code/install/gnu/hwloc/lib -lhwloc
gcc  -g -O3            -I/home/cjarcher/code/install/gnu/hwloc/include -I. -I./include -DN_CONSUMERS=${consumers} -DN_PRODUCERS=${producers} src/printme.c src/mpsc3.c  -o mpsc3 -lpthread -L/home/cjarcher/code/install/gnu/hwloc/lib -Wl,-rpath,/home/cjarcher/code/install/gnu/hwloc/lib -lhwloc
g++  -g -O3 -std=c++11 -I/home/cjarcher/code/install/gnu/hwloc/include -I. -I./include -DN_CONSUMERS=${consumers} -DN_PRODUCERS=${producers} src/printme.c src/mpsc5.cc -o mpsc5 -lpthread -L/home/cjarcher/code/install/gnu/hwloc/lib -Wl,-rpath,/home/cjarcher/code/install/gnu/hwloc/lib -lhwloc
gcc -std=c11 -g -O3 -I/home/cjarcher/code/install/gnu/hwloc/include -I. -I./include -DN_CONSUMERS=${consumers} -DN_PRODUCERS=${producers} src/printme.c src/lockrate.c -o lockrate -lpthread -L/home/cjarcher/code/install/gnu/hwloc/lib -Wl,-rpath,/home/cjarcher/code/install/gnu/hwloc/lib -lhwloc
