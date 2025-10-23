#!/bin/bash

echo "Compiles the Compiler"
echo "Starting Compiling"
base=TTC:

start=$(date +%s.%N)
g++ -std=c++23 -ofast -pipe -funroll-loops Compiler.cxx -o Compiler
end=$(date +%s.%N)


printf "%s %.3f\n" "$base" "$(awk -v s="$start" -v e="$end" 'BEGIN{print e-s}')"