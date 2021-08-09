#include "boost_custard.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <iostream>
#include <string>

using namespace boost::iostreams;

int main(int argc, char* argv[])
{
    filtering_istream fstr;
    std::string filename(argv[1]);
    if (boost::algorithm::iends_with(filename, ".gz")) {
        fstr.push(gzip_decompressor());
    } else if (boost::algorithm::iends_with(filename, ".bz2")) {
        fstr.push(bzip2_decompressor());
    } else if (boost::algorithm::iends_with(filename, ".tar")) {
        fstr.push(custard::tar_reader());
    } else if (boost::algorithm::iends_with(filename, ".tar.gz")) {
        fstr.push(custard::tar_reader());
    } else if (boost::algorithm::iends_with(filename, ".tar.bz2")) {
        fstr.push(custard::tar_reader());
    } else {
        std::cerr << "Unknown file suffix: " << filename << std::endl;
        return 1;
    }
    fstr.push(file_source(filename));

    while (true) {
        if (fstr.eof()) {
            std::cerr << "reached end of archive\n";
            break;
        }

        std::string fname, bsize;
        std::getline(fstr, fname, '\n');
        if (!fstr or fstr.eof()) {
            std::cerr << "Failed to get fname\n";
            break;
        }
        std::getline(fstr, bsize, '\n');
        size_t size = atol(bsize.c_str());
        std::cerr << "file: |" << fname << "|, size:|" << size << "|\n";

        std::string buf(size, 0);
        fstr.read(&buf[0], size);
        if (!fstr or fstr.eof()) {
            std::cerr << "eof after read body\n";
            break;
        }
        std::ofstream one(fname);
        one << buf;

        //std::cerr << buf << std::endl;

    }
    return 0;
}
