#include "custard_file.hpp"

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/bzip2.hpp>

#include <boost/algorithm/string.hpp>

#include <fstream>
#include <iostream>

using namespace boost::iostreams;

int main(int argc, char* argv[])
{
    std::ofstream fout(argv[1], std::ios_base::out | std::ios_base::binary);
    filtering_ostream out;

    std::string filename(argv[1]);
    if (boost::algorithm::iends_with(filename, ".gz")) {
        out.push(gzip_compressor());
    } else if (boost::algorithm::iends_with(filename, ".bz2")) {
        out.push(bzip2_compressor());
    } else if (boost::algorithm::iends_with(filename, ".tar")) {
        //No decompression filter needed
    } else {
        std::cerr << "Unknown file suffix: " << filename << std::endl;
        return 1;
    }
    out.push(fout);

    custard::File tar;
    for (size_t ind = 2; ind<argc; ++ind) {
        std::string realname(argv[ind]);
        std::string fname = realname;
        if (fname[0] == '/') {
            fname.erase(fname.begin());
        }
        std::ifstream ifs(realname, std::ifstream::ate | std::ifstream::binary);
        if (!ifs) {
            std::cerr << "No such file: " << realname << std::endl;
            continue;
        }
        auto siz = ifs.tellg();
        std::string buf(siz, 0);
        ifs.seekg(0);
        ifs.read(&buf[0], siz);
        ifs.close();

        std::cerr << "read " << realname << " " << siz << " bytes\n";
        auto got = tar.write_file(out, fname, buf.data(), siz);
        std::cerr << "wrote " << fname << " " << got << " bytes\n";
        assert(out);
        assert(got == siz);
        assert(tar.header().size() == 0);
    }
    tar.write_finish(out);
    out.flush();

    return 0;
}
