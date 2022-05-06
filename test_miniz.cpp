#include "miniz.h"

#include <string>
#include <fstream>

int main(int argc, char* argv[])
{
    const std::string zipfile = argv[1];

    for (int ind=2; ind < argc; ++ind) {
        const std::string member = argv[ind];
        
        std::ifstream fi(member, std::ifstream::ate | std::ifstream::binary);
        auto siz = fi.tellg();
        fi.seekg(0);
        std::string buf(siz, 0);
        fi.read((char*)buf.data(), buf.size());

        auto ok = mz_zip_add_mem_to_archive_file_in_place(
            zipfile.c_str(),
            member.c_str(),
            buf.data(),
            buf.size(),
            0, 0,
            MZ_BEST_COMPRESSION);
        if (!ok) {
            throw std::runtime_error("failed to write " + member);
        }
    }

    return 0;
}
