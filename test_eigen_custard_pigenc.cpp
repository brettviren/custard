#include <boost/iostreams/filtering_stream.hpp>

#include "custard_boost.hpp"

#include <Eigen/Core>
#include "pigenc_stream.hpp"


template<typename ArrayType>
void test_one(boost::iostreams::filtering_ostream& out,
              std::string arrname, int Nrows, int Ncols)
{
    std::vector<double> floats;
    std::vector<int> ints;
    ArrayType arr = ArrayType::Zero(Nrows, Ncols);
    for (int irow = 0; irow < Nrows; ++irow) {
        floats.push_back(irow*6.9);
        ints.push_back(irow);

        for (int icol=0; icol < Ncols; ++icol) {
            typename ArrayType::Scalar val = icol + irow*Ncols;
            arr(irow, icol) = val;
        }
    }

    custard::eigen_sink(out, arrname + ".npy", arr);
    custard::vector_sink(out, arrname + "_floats.npy", floats);
    custard::vector_sink(out, arrname + "_ints.npy", ints);
}


using Aff = Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

int main(int argc, char* argv[])
{
    std::string tarfile;
    if (argc > 1) {
        tarfile = argv[1];
    }
    else {
        tarfile = argv[0];
        tarfile += ".tar";
    }

    boost::iostreams::filtering_ostream out;
    custard::output_filters(out, tarfile);

    test_one<Aff>(out, "test1", 3,4);

    return 0;
}
       
