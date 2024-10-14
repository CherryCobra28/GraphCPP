#!/usr/bin/env sh

g++ test.cpp -I/usr/local/include/igraph -L/usr/local/lib -ligraph -o test

clang++ test.cpp -no-pie -I/usr/local/include/igraph -L/usr/local/lib -ligraph -o test
