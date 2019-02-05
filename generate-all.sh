#!/bin/sh

echo "const MidiProgram xg_set[] ="
echo "{"
./build/gm-xg-gs xg
echo "};"

echo ""

echo "const MidiProgram gs_set[] ="
echo "{"
./build/gm-xg-gs gs
echo "};"

echo ""

echo "const MidiProgram sc_set[] ="
echo "{"
./build/gm-xg-gs sc
echo "};"

echo ""

echo "const MidiProgram gm_set[] ="
echo "{"
./build/gm-xg-gs gm
echo "};"
