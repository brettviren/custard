#include "custard.hpp"

#include <iostream>
#include <sstream>
#include <fstream>

int main(int argc, char* argv[])
{
    std::ifstream fstr(argv[1]);
    if (!fstr) {
        std::cerr << "Bad file\n";
        return -1;
    }

    custard::File tar;
    const auto& th = tar.header();

    while (fstr) {

        const size_t siz = tar.read_start(fstr);

        if (! siz) {
            continue;
        }
        uint32_t usum = th.checksum();
        bool ok = usum == th.chksum();

        std::cerr << "filename: " << th.name()
                  << " size: " << siz
                  << " check: " << usum << "?" << ok
                  << " istar: " << th.is_ustar()
                  << std::endl;

        std::string buf(siz, '\0');
        auto got = tar.read_data(fstr, &buf[0], siz);
        if (got != siz) {
            std::cerr << "short read " << got << " < " << siz << std::endl;
        }
        std::cerr << buf << std::endl;

    }
    return 0;
}
