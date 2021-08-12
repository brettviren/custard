#ifndef eigen_custard_pigenc_hpp
#define eigen_custard_pigenc_hpp

#include "pigenc.hpp"

namespace custard {

#ifdef EIGEN_CORE_H
    /// Sink an eigen type array as Numpy data into a tar stream.
    ///
    /// The arname is the file name to give in the tar archive stream.
    /// Typically it should end in .npy.
    ///
    /// NOTE: you MUST #include <Eigen/Core>, or other header
    /// providing eigen-style array types before #include'ing this
    /// header in order to use this function.  
    template<typename ArrayType>
    std::ostream& eigen_sink(std::ostream& tar_stream,
                             const std::string& arname,
                             const ArrayType& array)
    {
        using Scalar = typename ArrayType::Scalar;

        std::vector<size_t> shape;
        shape.push_back(array.rows());
        shape.push_back(array.cols());
        const size_t nelem = shape[0]*shape[1];

        const bool fortran_order = !(ArrayType::Flags & Eigen::RowMajorBit);

        // need to divine dtype and shape (and size)
        auto dt = pigenc::dtype<Scalar>();
        auto pyhead = pigenc::make_header(dt, shape, fortran_order);

        const Scalar* array_data = array.data();
        const size_t array_size = nelem*sizeof(Scalar);

        const size_t tar_size = pyhead.size() + array_size;

        tar_stream << arname << "\n" << tar_size << "\n";
        tar_stream.write(pyhead.data(), pyhead.size());
        tar_stream.write((char*)array_data, array_size);

        return tar_stream;
    }
#endif // EIGEN_CORE_H

    /// Sink a vectgor to a tar stream.
    ///
    /// The arname is the file name to give in the tar archive stream.
    /// Typically it should end in .npy.
    template<typename VectorType>
    std::ostream& vector_sink(std::ostream& tar_stream,
                             const std::string& arname,
                              const VectorType& vec)
    {
        using Scalar = typename VectorType::value_type;
        std::vector<size_t> shape;
        shape.push_back(vec.size());
        const bool fortran_order = false;
        const size_t nelem = shape[0]*shape[1];

        auto dt = pigenc::dtype<Scalar>();
        auto pyhead = pigenc::make_header(dt, shape, fortran_order);

        const Scalar* vec_data = vec.data();
        const size_t vec_size = nelem*sizeof(Scalar);

        const size_t tar_size = pyhead.size() + vec_size;

        tar_stream << arname << "\n" << tar_size << "\n";
        tar_stream.write(pyhead.data(), pyhead.size());
        tar_stream.write((char*)vec_data, vec_size);

        return tar_stream;
    }
    

}

#endif
