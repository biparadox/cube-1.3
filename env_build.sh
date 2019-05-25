#!/bin/bash
cd cubelib
make clean
make
cd ../proc/main
make clean
make
cd ../src
make clean
make
cd ../..
