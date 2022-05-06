#include "miniz.h"

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
int main(int argc, char* argv[])
{
    const std::string zipfile = argv[1];

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));

    // such long names you have, granny.
    const int nosort = MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY;
    if (!mz_zip_reader_init_file(&zip, zipfile.c_str(), nosort)) {
        std::cerr << "Failed to open " << zipfile << std::endl;
        return 1;
    }
 
    size_t nfiles = mz_zip_reader_get_num_files(&zip);
    std::cerr << zipfile << " holds " << nfiles << " files\n";

    for (size_t ifile=0; ifile<nfiles; ++ifile) {
        
        mz_zip_archive_file_stat fs;
        if (!mz_zip_reader_file_stat(&zip, ifile, &fs)) {
            std::cerr << "failed to stat\n";
            mz_zip_reader_end(&zip);
            return 1;
        }

        bool isdir = mz_zip_reader_is_file_a_directory(&zip, ifile);

        std::cerr << fs.m_filename
                  << ": comment: \"" << fs.m_comment
                  << "\" uncompressed: " << fs.m_uncomp_size
                  << " compressed: " << fs.m_comp_size
                  << " isdir: " << isdir << "\n";

        if (isdir) {
            continue;
        }

        std::vector<char> dat(fs.m_uncomp_size, 0);
        auto ok = mz_zip_reader_extract_to_mem(&zip, ifile, dat.data(), dat.size(), 0);
        if (!ok) {
            std::cerr << "extraction failed\n";
            return 1;
        }
    }

    mz_zip_reader_end(&zip);

    return 0;
}
