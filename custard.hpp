#ifndef custard_hpp
#define custard_hpp

#include <istream>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sstream>
#include <algorithm>

#include <iostream>             // tmp debug

// This and other from
// https://techoverflow.net/2013/03/29/reading-tar-files-in-c/
// Converts an ascii digit to the corresponding number
#define ASCII_TO_NUMBER(num) ((num)-48) 

namespace custard {


    inline
    std::string encode_octal(int64_t val, int odigits) {
        std::stringstream ss;
        ss << std::oct << val;
        std::string s = ss.str();
        while (s.size() < odigits) {
            s.insert(0, "0");
        }
        return s;
    }

    inline
    void encode_octal_copy(char* var, int64_t val, int odigits) {
        auto s = encode_octal(val, odigits);
        memcpy(var, s.data(), odigits);
    }

    inline
    uint64_t decode_octal(const char* data, size_t size) {
        unsigned char* currentPtr = (unsigned char*) data + size;
        uint64_t sum = 0;
        uint64_t currentMultiplier = 1;
        unsigned char* checkPtr = currentPtr;

        for (; checkPtr >= (unsigned char*) data; checkPtr--) {
            if ((*checkPtr) == 0 || (*checkPtr) == ' ') {
                currentPtr = checkPtr - 1;
            }
        }
        for (; currentPtr >= (unsigned char*) data; currentPtr--) {
            sum += ASCII_TO_NUMBER(*currentPtr) * currentMultiplier;
            currentMultiplier *= 8;
        }
        return sum;
    }

    struct Header
    {                         /* byte offset */
        char _name[100];               /*   0 */
        char _mode[8];                 /* 100 */
        char _uid[8];                  /* 108 */
        char _gid[8];                  /* 116 */
        char _size[12];                /* 124 */
        char _mtime[12];               /* 136 */
        char _chksum[8];               /* 148 */
        char _typeflag;                /* 156 */
        char _linkname[100];           /* 157 */
        char _magic[6];                /* 257 */
        char _version[2];              /* 263 */
        char _uname[32];               /* 265 */
        char _gname[32];               /* 297 */
        char _devmajor[8];             /* 329 */
        char _devminor[8];             /* 337 */
        char _prefix[155];             /* 345 */
        char _padding[12];             /* 500 */
                                       /* 512 */

        void prepare(std::string filename, size_t siz) {
            memset((char*)this, '\0', 512);
            size_t fns = std::min(filename.size(), 100UL);
            memcpy(_name, filename.data(), fns);
            encode_octal_copy(_mode, 0444, 8);
            auto uid = geteuid();
            encode_octal_copy(_uid, uid, 8);
            auto gid = getegid();
            encode_octal_copy(_gid, gid, 8);
            encode_octal_copy(_size, siz, 12);
            encode_octal_copy(_mtime, time(0), 12);
            memcpy(_magic, "ustar", 5);
            memset(_version, '0', 2);
            auto* pw = getpwuid (uid);
            if (pw) {
                std::string uname(pw->pw_name);
                memcpy(_uname, uname.data(), std::min(uname.size(), 32UL));
            }
            auto* gw = getgrgid(gid);
            if (gw) {
                std::string gname(gw->gr_name);
                memcpy(_gname, gname.data(), std::min(gname.size(), 32UL));
            }
            checksum(true);
        }


        void write(std::ostream& so, std::string filename,
                   const char* data, size_t datasize) {
            prepare(filename, datasize);
            so.write((char*)this, 512);
            so.write(data, datasize);
        }

        // Read just next header, assuming stream is correctly
        // positioned.
        void read(std::istream& si) {
            si.read((char*)this, 512);
        }

        // Seek the stream to next multiple of 512 bytes
        void seek_next(std::istream& si) {
            const size_t padding = 512 - size()%512;
            si.seekg(padding, si.cur);
        }

        // Read into buf assuming buf is at least as large as size
        void read_file(std::istream& si, char* buf) {
            si.read(buf, size());
            seek_next(si);
        }

        // Read and return new memory holding file.  Caller must delete[].
        // Note, file not guarateed to be text nor NULL \0 terminated.
        char* read_file_new(std::istream& si) {
            char* buf = new char[size()];
            si.read(buf, size());
            seek_next(si);
            return buf;
        }
            
        bool is_ustar() const {
            return memcmp("ustar", _magic, 5) == 0;
        }

        // Return true if stored checksum matches calculation.
        // If update is true, set the checksum entry in the header.
        bool checksum(bool update=false)  {
            int64_t usum{0}, isum{};
            for (size_t ind = 0; ind < 148; ++ind) {
                usum += ((unsigned char*) this)[ind];
                isum += ((signed char*) this)[ind];
            }
            // the checksum field itself is meant to be spaces
            for (size_t ind = 0; ind < 8; ++ind) {
                usum += ((unsigned char) ' ');
                isum += ((signed char) ' ');
            }                
            for (size_t ind = 156; ind < 512; ++ind) {
                usum += ((unsigned char*) this)[ind];
                isum += ((signed char*) this)[ind];
            }
            int64_t have = decode_octal(_chksum, 8);
            if (update) {
                auto c = encode_octal(usum, 8);
                memcpy(_chksum, c.data(), 8);
            }
            return (have == usum || have == isum);
        }

        size_t size() const {
            return decode_octal(_size, 12);
        }

    };
    
}

#endif
