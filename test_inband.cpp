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
    filtering_ostream out;

    std::string filename(argv[1]);
    if (boost::algorithm::iends_with(filename, ".gz")) {
        out.push(gzip_compressor());
    } else if (boost::algorithm::iends_with(filename, ".bz2")) {
        out.push(bzip2_compressor());
    } else if (boost::algorithm::iends_with(filename, ".tar")) {
        out.push(custard::output_filter());
    } else if (boost::algorithm::iends_with(filename, ".tar.gz")) {
        out.push(custard::output_filter());
    } else if (boost::algorithm::iends_with(filename, ".tar.bz2")) {
        out.push(custard::output_filter());
    } else {
        std::cerr << "Unknown file suffix: " << filename << std::endl;
        return 1;
    }
    out.push(file_sink(filename));

    for (size_t ind = 2; ind<argc; ++ind) {
        std::string realname(argv[ind]);
        std::cerr << "archive file: " << realname << std::endl;
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

        out << fname << "\n" << siz << "\n" << buf;
    }

    out.flush();
    return 0;
}


int _main()
{
    filtering_ostream out;
    out.push(custard::output_filter());
    out.push(file_sink("test_inband.tar"));
    
    std::string msg1 = "some file content\n";
    std::string msg2 = "SOME MORE FILE CONTENT\n";

    out << "fileA.txt\n" << msg1.size() << "\n" << msg1;
    out << "fileB.txt\n" << msg2.size() << "\n" << msg2;
    out.flush();

    return 0;
}
