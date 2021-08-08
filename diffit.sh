#!/bin/bash

set -e
set -x

prog=test_inband

files=(GPL GPL2 $HOME/unnamed.jpg)

g++ -o $prog $prog.cpp -lboost_iostreams

touch ${files[*]}
./$prog     cus.tar ${files[*]}
tar -b2 -cf gnu.tar ${files[*]}

od -a cus.tar > cus.od
od -a gnu.tar > gnu.od
diff --color=always -U 6 cus.od gnu.od |less -R

ls -l cus.tar
tar -tvf cus.tar
ls -l gnu.tar
tar -tvf gnu.tar


