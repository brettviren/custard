// test custard.hpp
//
// Beware, the code here would NOT be ideal for a real app 

#include "custard.hpp"
#include <fstream>
#include <string>

int unpack(std::string archive)
{
    custard::Header head;
    std::ifstream fi(archive);
    while (fi) {
        fi.read(head.as_bytes(), 512);
        if (!fi) return -1;

        std::string path = head.name();
        while (path[0] == '/') {
            // At leat pretend to be a little secure
            path.erase(path.begin());
        }
        std::ofstream fo(path);
        if (!fo) return -1;

        // This is NOT smart on large files!
        std::string buf(head.size(), 0);
        fi.read((char*)buf.data(), buf.size());
        if (!fi) return -1;
        fo.write(buf.data(), buf.size());
        if (!fo) return -1;

        // get past padding
        size_t npad = 512 - head.size() % 512;
        fi.seekg(npad, fi.cur);
        if (!fi) return -1;

    }
    return 0;
}

int pack(std::string archive, int nmembers, char* member[])
{
    std::ofstream fo(archive);
    for (int ind = 0; ind<nmembers; ++ind) {
        std::string path(member[ind]);

        std::ifstream fi(path, std::ifstream::ate | std::ifstream::binary);        
        if (!fi) return -1;

        auto siz = fi.tellg();
        fi.seekg(0);
        if (!fi) return -1;

        // note, real tar preserves mtime, uid, etc.
        custard::Header head(path, siz);

        fo.write(head.as_bytes(), 512);
        if (!fo) return -1;

        std::string buf(head.size(), 0);
        fi.read((char*)buf.data(), buf.size());
        if (!fi) return -1;

        fo.write(buf.data(), buf.size());
        if (!fo) return -1;

        size_t npad = 512 - head.size() % 512;
        std::string pad(npad, 0);
        fo.write(pad.data(), pad.size());
        if (!fo) return -1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " archive.tar [file ...]" << std::endl;
        std::cerr << " with no files, extract archive.tar, otherwise produce it\n";
        return -1;
    }

    if (argc == 2) {
        return unpack(argv[1]);
    }

    return pack(argv[1], argc-2, argv+2);
}

