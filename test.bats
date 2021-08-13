#!/usr/bin/env bats

do_prep () {
    name="$1" ; shift    
    texe=$(realpath bin/$name)    
    outd=test/$name
    rm -rf $outd
    mkdir -p $outd
}
do_write () {
    name="$1" ; shift
    do_prep $name

    cp *.hpp $outd/
    cd $outd
    mkdir -p cus gnu

    run $texe   cus.tar *.hpp
    echo "$output"
    [ "$status" -eq 0 ]
    [ -s cus.tar ]

    # check we can untar and get back original files
    tar -C cus -xf cus.tar
    for one in *.hpp
    do
        diff -u $one cus/$one
    done

    # check we can make nearly identical tar file with GNU
    tar -b2 -cf gnu-full.tar *.hpp
    # our tar files do not pad as much GNU
    dd if=gnu-full.tar of=gnu.tar bs=1 count=$(stat -c %s cus.tar)

    od -a cus.tar > cus.od
    od -a gnu.tar > gnu.od
    diff -u cus.od gnu.od 

    # now check gzip compression.
    run $texe   cus.tar.gz *.hpp
    echo "$output"
    [ "$status" -eq 0 ]
    [ -s cus.tar.gz ]
    gzip -dc cus.tar.gz > cus2.tar
    od -a cus2.tar > cus2.od
    diff -u cus.od cus2.od 

    # now check bzip2 compression.
    run $texe   cus.tar.bz2 *.hpp
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
    do_prep $name

    tar -cf $outd/gnu.tar *.hpp 
    tar -czf $outd/gnu.tar.gz *.hpp 
    tar -cjf $outd/gnu.tar.bz2 *.hpp 

    cd $outd/

    for one in gnu.tar gnu.tar.gz gnu.tar.bz2
    do
        rm -f *.hpp
        run $texe $one
        echo "$output"
        [ "$status" -eq 0 ]
        [ -s custard.hpp ]
        for n in *.hpp
        do
            diff $n ../../$n
        done
    done
    date > okay
}

@test "test_custard_write" {
    do_write test_custard_write
}

@test "test_custard_read" {
    do_prep test_custard_read

    tar -cf $outd/gnu.tar *.hpp 

    cd $outd/

    for one in gnu.tar
    do
        rm -f *.hpp
        run $texe $one
        echo "$output"
        [ "$status" -eq 0 ]
        [ -s custard.hpp ]
        for n in *.hpp
        do
            diff $n ../../$n
        done
    done
    date > okay

}

@test "test_boost_custard_write" {
    do_write test_boost_custard_write
}

@test "test_boost_custard_read" {
    do_read test_boost_custard_read
}

do_pydump () {
    tar="$1" ; shift
    npy="$1" ; shift
    run python3 -c 'import io, numpy, tarfile; \
tar="'$tar'"; \
npy="'$npy'"; \
t = tarfile.open(tar); \
a = io.BytesIO(); \
a.write(t.extractfile(npy).read()); \
a.seek(0); \
arr = numpy.load(a); \
print(tar, npy); \
print(arr.shape, arr.size, arr.dtype); \
print(arr.flags); \
print(arr)'
    echo "$output"
    [ "$status" -eq 0 ]
}

@test "test_eigen_custard_pigenc" {
    do_prep test_eigen_custard_pigenc

    eigchk=$(realpath eigchk.py)
    cd $outd

    run $texe cus.tar
    echo "$output"
    [ "$status" -eq 0 ]
    [ -s cus.tar ]
    
    do_pydump cus.tar test1.npy
    [ -n "$(echo $output | grep '(3, 4) 12 float32')" ]
    [ -n "$(echo $output | grep 'C_CONTIGUOUS : True')" ]
    [ -n "$(echo $output | grep 'F_CONTIGUOUS : False')" ]
    [ -n "$(echo $output | grep 'ALIGNED : True')" ]

    do_pydump cus.tar test1_floats.npy
    [ -n "$(echo $output | grep '(3,) 3 float64')" ]
    [ -n "$(echo $output | grep 'C_CONTIGUOUS : True')" ]
    [ -n "$(echo $output | grep 'F_CONTIGUOUS : True')" ]
    [ -n "$(echo $output | grep 'ALIGNED : True')" ]

    do_pydump cus.tar test1_ints.npy
    [ -n "$(echo $output | grep '(3,) 3 int32')" ]
    [ -n "$(echo $output | grep 'C_CONTIGUOUS : True')" ]
    [ -n "$(echo $output | grep 'F_CONTIGUOUS : True')" ]
    [ -n "$(echo $output | grep 'ALIGNED : True')" ]

    #false

    date > okay


}

@test "test_pigenc" {
    do_prep test_pigenc

    cd $outd

    run $texe cus.npy
    echo "$output"
    [ "$status" -eq 0 ]
    [ -s cus.npy ]

    date > okay

}
