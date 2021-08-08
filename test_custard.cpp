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

    custard::Header th;

    while (fstr) {

        th.read(fstr);
        const size_t siz = th.size();

        if (th.size()) {


            std::cerr << "Header for file: " << th._name << ": "
                      << siz << " bytes,"
                      << " check: " << th.checksum()
                      << " istar: " << th.is_ustar()
                      << std::endl;
            std::cerr << "\t _cksum: |" << std::string(th._chksum, 8)
                      << "| _size: " << std::string(th._size, 12)
                      << std::endl;
            th.checksum(true);
            std::cerr << "\t _cksum: |" << std::string(th._chksum, 8)
                      << "| _size: " << std::string(th._size, 12)
                      << std::endl;


            char* buf = th.read_file_new(fstr);
            std::string s = buf;
            delete[] buf;

            // std::string s(siz, ' ');
            // fstr.read(&s[0], siz);

            //std::cerr << s << std::endl;
        }
    }
    return 0;
}
