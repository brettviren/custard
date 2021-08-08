#!/bin/bash

set -e
set -x

files=(GPL GPL2 $HOME/unnamed.jpg)

g++ -g -o test_custard_write test_custard_write.cpp -lboost_iostreams
touch ${files[*]}
./test_custard_write cus.tar ${files[*]}
tar -b2 -cf          gnu.tar ${files[*]}
od -a cus.tar > cus.od
od -a gnu.tar > gnu.od
diff --color=always -U 6 cus.od gnu.od |less -R
ls -l cus.tar
tar -tvf cus.tar
ls -l gnu.tar
tar -tvf gnu.tar


