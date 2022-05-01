#!/bin/bash

for ext in zip tar tgz tix
do
    echo
    echo $ext
    arc=test-speed.$ext
    rm -rf $arc
    time ./bin/test_custard_boost $arc $@ > $arc.log 2>&1 || exit 1
    if [ -n "$(grep 'write error' $arc.log)" ] ; then
        echo "failed"
    fi
done
