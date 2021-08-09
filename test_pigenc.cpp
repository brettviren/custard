#include "pigenc.hpp"

#include <fstream>
#include <iostream>

int main(int argc, char* argv[])
{

    assert(pigenc::dtype<int>() == "<i4");
    assert(pigenc::dtype<short>() == "<i2");
    assert(pigenc::dtype<int8_t>() == "<i1");
    assert(pigenc::dtype<char>() == "c");
    assert(pigenc::dtype<float>() == "<f4");
    assert(pigenc::dtype<double>() == "<f8");

    {
        std::vector<size_t> shape{2,2};
        auto head = pigenc::make_header("<i4", shape);
        std::cerr << head << std::endl;    

        std::ofstream out("test_pigenc.npy");
        int data[4] = {0,1,2,3};
        pigenc::write_data(out, data, {2,2});
    }

    {

        std::ifstream fstr("test_pigenc.npy");

        auto head = pigenc::read_header(fstr);
        std::cerr << head.dump(4) << std::endl;

        size_t size=1;
        for (auto s : head["shape"]) {
            int one = s;
            size *= one;
        }
        int* data = new int[size];
    
        fstr.read((char*)data, size*sizeof(int));
        for (int ind=0; ind<4; ++ind) {
            std::cerr << ind << ": " << data[ind] << std::endl;
        }
    }

    return 0;
}

