#include "miniz.h"

#include <string>
#include <fstream>
#include <iostream>

int main(int argc, char* argv[])
{
    const std::string zipfile = argv[1];

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));

    if (!mz_zip_writer_init_file(&zip, zipfile.c_str(), 0)) {
        std::cerr << "Failed to open " << zipfile << std::endl;
        return 1;
    }
 
    for (int ind=2; ind < argc; ++ind) {
        const std::string member = argv[ind];
        
        std::ifstream fi(member, std::ifstream::ate | std::ifstream::binary);
        auto siz = fi.tellg();
        fi.seekg(0);
        std::string buf(siz, 0);
        fi.read((char*)buf.data(), buf.size());

        auto ok = mz_zip_writer_add_mem(
            &zip,
            member.c_str(),
            buf.data(),
            buf.size(),
            MZ_BEST_SPEED);
        if (!ok) {
            throw std::runtime_error("failed to write " + member);
        }
    }
    mz_zip_writer_finalize_archive(&zip);
    mz_zip_writer_end(&zip);
    return 0;
}
