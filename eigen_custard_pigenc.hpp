#ifndef eigen_custard_pigenc_hpp
#define eigen_custard_pigenc_hpp

#include <Eigen/Core>
#include "pigenc.hpp"

namespace custard {

    /// Sink an eigen type array as Numpy data into a tar stream.  The
    /// arname is the file name to give in the tar archive stream.
    /// Typically it should end in .npy.
    template<typename ArrayType>
    std::ostream& eigen_sink(std::ostream& tar_stream,
                             const std::string& arname,
                             const ArrayType& array)
    {
        using Scalar = typename ArrayType::Scalar;

        std::vector<size_t> shape;
        shape.push_back(array.rows());
        shape.push_back(array.cols());
        size_t nelem = shape[0]*shape[1];

        bool fortran_order = !(ArrayType::Flags & Eigen::RowMajorBit);

        // need to divine dtype and shape (and size)
        auto dt = pigenc::dtype<Scalar>();
        auto pyhead = pigenc::make_header(dt, shape, fortran_order);

        const Scalar* array_data = array.data();
        size_t array_size = nelem*sizeof(Scalar);

        size_t tar_size = pyhead.size() + array_size;

        tar_stream << arname << "\n" << tar_size << "\n";
        tar_stream.write(pyhead.data(), pyhead.size());
        tar_stream.write((char*)array_data, array_size);

        return tar_stream;
    }

}

#endif
