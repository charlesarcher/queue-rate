#!/bin/bash

autoreconf -ivf

cd jemalloc && ./autogen.sh && cd -
