#!/bin/bash

set -e
set -x

rm -f test_eigen_custard_pigenc.tar
g++ -I /usr/include/eigen3 -I../ -o test_eigen_custard_pigenc test_eigen_custard_pigenc.cpp -l boost_iostreams
./test_eigen_custard_pigenc
tar -xvf test_eigen_custard_pigenc.tar

python -c 'import numpy; \
       fp = numpy.array( numpy.arange(12).reshape(3,4), dtype="<f4"); \
       numpy.save("frompy.npy", fp);'
       

od -a test1.npy > test1.od
od -a frompy.npy > frompy.od
diff --color=always -u test1.od frompy.od

python -c 'import io, numpy, tarfile; \
       t = tarfile.open("test_eigen_custard_pigenc.tar"); \
       a = io.BytesIO(); \
       a.write(open("frompy.npy","rb").read()); \
       print("frompy:\n",a.getbuffer().tobytes()); \
       b = io.BytesIO(); \
       b.write(t.extractfile("test1.npy").read()); \
       print("test1:\n",b.getbuffer().tobytes()); \
       for i,(x,y) in enumerate(zip(a.getbuffer().tobytes(), b.getbuffer().tobytes())): \
           print(i,x,y) \
       a.seek(0); \
       arr = numpy.load(a); \
       print(arr.shape, arr.dtype)'
       


