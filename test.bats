#!/usr/bin/env bats

# make a working directory for the test exec and set some vars
do_prep () {
    name="$1" ; shift    
    extra="$1"
    texe=$(realpath bin/$name)    
    outd=test/${name}_${extra}
    rm -rf $outd
    mkdir -p $outd
    echo $texe
    echo $outd
}

# many tests have same write pattern: $text foo.tar file ...
do_write () {
    name="$1" ; shift
    comp="$1" ; shift
    ext="${1:-hpp}"             # foder file type

    do_prep $name write_$comp

    cp *.${ext} $outd/
    cd $outd
    mkdir -p cus gnu

    run $texe cus.tar *.${ext}
    echo "$output"
    [ "$status" -eq 0 ]
    [ -s cus.tar ]
    ls -lR
    [ $(stat -c %s cus.tar) -gt 1024 ]

    # check we can untar and get back original files
    tar -C cus -xvf cus.tar

    for one in *.${ext}
    do
        diff -u $one cus/$one
    done

    # check we can make nearly identical tar file with GNU
    tar -b2 -cf gnu-full.tar *.${ext}
    # our tar files do not pad as much GNU
    dd if=gnu-full.tar of=gnu.tar bs=1 count=$(stat -c %s cus.tar)

    od -a cus.tar > cus.od
    od -a gnu.tar > gnu.od
    diff -u cus.od gnu.od 

    if [ "$comp" = "no" ] ; then
        return
    fi

    # now check gzip compression.
    run $texe   cus.tar.gz *.${ext}
    echo "$output"
    [ "$status" -eq 0 ]
    [ -s cus.tar.gz ]
    gzip -dc cus.tar.gz > cus2.tar
    od -a cus2.tar > cus2.od
    diff -u cus.od cus2.od 

    # now check bzip2 compression.
    run $texe   cus.tar.bz2 *.${ext}
    echo "$output"
    [ "$status" -eq 0 ]
    [ -s cus.tar.bz2 ]
    bzip2 -dc cus.tar.bz2 > cus3.tar
    od -a cus3.tar > cus3.od
    diff -u cus.od cus3.od 

    date > okay
}


do_read () {
    name="$1" ; shift
    comp="$1" ; shift
    ext="${1:-hpp}"             # fodder extension
    do_prep $name read_$comp

    tar -cf $outd/gnu.tar *.${ext}
    if [ "$comp" = "yes" ] ; then
        tar -czf $outd/gnu.tar.gz *.${ext}
        tar -cjf $outd/gnu.tar.bz2 *.${ext}
    fi

    cd $outd/
    pwd
    echo $texe

    tars=(gnu.tar)
    if [ "$comp" = "yes" ] ; then
        tars=(gnu.tar gnu.tar.bz2 gnu.tar.gz)
    fi


    for one in ${tars[*]}
    do
        echo $one
        rm -f *.${ext}
        pwd
        echo $texe $one
        run $texe $one
        echo "$output"
        [ "$status" -eq 0 ]
        for n in *.${ext}
        do
            ls -l $n ../../$n
            md5sum $n ../../$n
            diff -u $n ../../$n
        done
    done
    date > okay
}

@test "test_custard write" {
    do_write test_custard no hpp
}

@test "test_custard read" {
    do_read test_custard no hpp
}

@test "test_custard_boost write" {
    do_write test_custard_boost yes hpp
}

@test "test_custard_boost read" {
    do_read test_custard_boost yes hpp
}


@test "test_custard_boost_pigenc_eigen write" {
    do_write_py test_custard_boost_pigenc_eigen comp
}

@test "test_custard_boost_pigenc_eigen read" {
    do_read_py test_custard_boost_pigenc_eigen comp
}
