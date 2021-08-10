#!/bin/bash

set -e
set -x

prog=${1:-test_boost_custard_write}

files=(COPYING custard.hpp boost_custard.hpp)

rm -rf diffit
mkdir -p diffit
cp ${files[*]} diffit/

g++ -o $prog $prog.cpp -lboost_iostreams

cd diffit/
touch ${files[*]}
../$prog     cus.tar ${files[*]}
tar  -b2 -cf gnu.tar ${files[*]}

od -a cus.tar > cus.od
od -a gnu.tar > gnu.od
diff --color=always -U 6 cus.od gnu.od |less -R

ls -l cus.tar
tar -tvf cus.tar
ls -l gnu.tar
tar -tvf gnu.tar

cd ..
