#include "pigenc.hpp"

#include <fstream>
#include <iostream>

int main(int argc, char* argv[])
{
    std::ifstream fstr(argv[1]);

    auto dat = pigenc::read_header(fstr);
    std::cerr << dat.dump(4) << std::endl;

    auto h = pigenc::make_header("<i8", {2,2});
    std::cerr << h << std::endl;    

    assert(pigenc::dtype<int>() == "<i4");
    assert(pigenc::dtype<short>() == "<i2");
    assert(pigenc::dtype<int8_t>() == "<i1");
    assert(pigenc::dtype<char>() == "c");
    assert(pigenc::dtype<float>() == "<f4");
    assert(pigenc::dtype<double>() == "<f8");

    std::ofstream out(argv[2]);
    int data[4] = {0,1,2,3};
    pigenc::write_data(out, data, {2,2});

    return 0;


}

