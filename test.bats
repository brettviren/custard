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

    tar -b2 -cf gnu.tar *.hpp

    tar -C cus -xf cus.tar
    for one in *.hpp
    do
        diff -q $one cus/$one
    done

    od -a cus.tar > cus.od
    od -a gnu.tar > gnu.od
    diff cus.od gnu.od 
    date > okay
}

do_read () {
    name="$1" ; shift
    do_prep $name

    tar -cf $outd/gnu.tar *.hpp 

    cd $outd/

    run $texe gnu.tar
    echo "$output"
    [ "$status" -eq 0 ]
    [ -s custard.hpp ]

    for n in *.hpp
    do
        diff $n ../../$n
    done
    date > okay
}

@test "test_custard_write" {
    do_write test_custard_write
}

@test "test_custard_read" {
    do_read test_custard_read
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

    run $texe cus.tar
    echo "$output"
    [ "$status" -eq 0 ]
    [ -s cus.tar ]

    date > okay

}
