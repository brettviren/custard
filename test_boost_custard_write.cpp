#include "custard_boost.hpp"

#include <boost/iostreams/filtering_stream.hpp>

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    boost::iostreams::filtering_ostream out;
    std::string filename(argv[1]);
    custard::output_filters(out, filename);
    if (out.empty()) {
        std::cerr << "failed to understand filename: "
                  << filename << std::endl;
        return -1;
    }

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

    out.pop();                  // remove the file to close.
    return 0;
}

