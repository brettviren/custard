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

#include <iostream>
#include <cassert>

// This and other from
// https://techoverflow.net/2013/03/29/reading-tar-files-in-c/
// Converts an ascii digit to the corresponding number
#define ASCII_TO_NUMBER(num) ((num)-48) 

namespace custard {


    inline
    void encode_chksum(char* var, int64_t val) {
        std::stringstream ss;
        ss << std::oct << val;
        std::string s = ss.str();
        if (s[0] != '0' and s.size() < 8) {
            s.insert(0, "0");
        }
        if (s.size() % 2) {
            s.insert(0, "0");
        }
        if (s.size() < 8) {
            s += '\0';
        }
        while (s.size() < 8) {
            s += ' ';
        }
        memcpy(var, s.data(), 8);
    }
    inline
    void encode_null(char* var, int64_t val, int odigits) {
        std::stringstream ss;
        ss << std::oct << val;
        std::string s = ss.str();
        while (s.size() < odigits-1) {
            s.insert(0, "0");
        }
        s += '\0';
        memcpy(var, s.data(), odigits);
    }
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

    class Header
    {                         /* byte offset */
        char name_[100];               /*   0 */
        char mode_[8];                 /* 100 */
        char uid_[8];                  /* 108 */
        char gid_[8];                  /* 116 */
        char size_[12];                /* 124 */
        char mtime_[12];               /* 136 */
        char chksum_[8];               /* 148 */
        char typeflag_;                /* 156 */
        char linkname_[100];           /* 157 */
        char magic_[6];                /* 257 */
        char version_[2];              /* 263 */
        char uname_[32];               /* 265 */
        char gname_[32];               /* 297 */
        char devmajor_[8];             /* 329 */
        char devminor_[8];             /* 337 */
        char prefix_[155];             /* 345 */
        char padding_[12];             /* 500 */
                                       /* 512 */
      public:
        Header(std::string filename="", size_t siz=0)
        {
            init(filename, siz);
        }
        void clear()
        {
            memset((char*)this, '\0', 512);
        }
        void init(std::string filename="", size_t siz=0)
        {
            clear();
            memcpy(name_, filename.data(), std::min(filename.size(), 100UL));
            encode_null(mode_, 0644, 8);
            auto uid = geteuid();
            encode_null(uid_, uid, 8);
            auto gid = getegid();
            encode_null(gid_, gid, 8);
            encode_null(size_, siz, 12);
            encode_null(mtime_, time(0), 12);
            typeflag_ = '0';
            memcpy(magic_, "ustar ", 6);
            // old gnu magic over writes version.
            version_[0] = ' ';  
            auto* pw = getpwuid (uid);
            if (pw) {
                std::string uname(pw->pw_name);
                memcpy(uname_, uname.data(), std::min(uname.size(), 32UL));
            }
            auto* gw = getgrgid(gid);
            if (gw) {
                std::string gname(gw->gr_name);
                memcpy(gname_, gname.data(), std::min(gname.size(), 32UL));
            }
            set_chksum(checksum());
        }

        std::string name() const {
            return name_;
        }
        uint32_t chksum() const {
            return (uint32_t) decode_octal(chksum_, 8);
        }

        // Calculate and return checksum of header
        uint32_t checksum() const {
            uint32_t usum{0};
            for (size_t ind = 0; ind < 148; ++ind) {
                usum += ((unsigned char*) this)[ind];
            }
            // the checksum field itself is must be interpreted as
            // spaces.
            for (size_t ind = 0; ind < 8; ++ind) {
                usum += ((unsigned char) ' ');
            }                
            for (size_t ind = 156; ind < 512; ++ind) {
                usum += ((unsigned char*) this)[ind];
            }
            return usum;
        }

        void set_chksum(uint32_t usum) {
            encode_chksum(chksum_, usum);
        }

        bool is_ustar() const {
            return memcmp("ustar", magic_, 5) == 0;
        }

        size_t size() const {
            return decode_octal(size_, 12);
        }

        const char* as_bytes() const {
            return reinterpret_cast<const char*>(this);
        }
        char* as_bytes()  {
            return reinterpret_cast<char*>(this);
        }

    };
    
    class File {
        size_t loc{0}; // current read/write file location
        Header head;
      public:

        const Header& header() const {
            return head;
        }

        void clear() {
            loc = 0;
            head.clear();
        }

        // Start a file of given file name and target file size.
        void write_start(std::ostream& so, std::string filename,
                         size_t datasize)
        {
            loc = 0;
            head.init(filename, datasize);
            so.write((char*)head.as_bytes(), 512);
        }

        // Write some data to current file, not overflowing file size.
        // Return how data consumed.
        size_t write_data(std::ostream& so, const char* data,
                        size_t datasize)
        {
            size_t remain = head.size() - loc;
            size_t take = std::min(datasize, remain);
            so.write(data, take);
            loc += take;
            if (loc == head.size()) {
                write_pad(so);
                clear();
            }

            return take;
        }

        // Start a file and write entire or intial file data.  Return
        // how much of data consumed.
        size_t write_file(std::ostream& so, std::string filename,
                        const char* data, size_t datasize)
        {
            write_start(so, filename, datasize);
            size_t ret = write_data(so, data, datasize);
            return ret;
        }

        // Write cpad to next 512 boundary
        void write_pad(std::ostream& so)
        {
            size_t pad = 512 - loc%512;
            if (pad == 0) {
                return;
            }
            for (size_t ind=0; ind<pad; ++ind) {
                so.put('\0');
            };
        }
        
        // GNU manual on tar file format: at the end of the archive
        // file there are two 512-byte blocks filled with binary zeros
        // as an end-of-file marker.
        //
        // GNU tar, actual: lol, nah, I'mma gonna write 20! lololulz
        //
        // With only 2, GNU tar reads the archive fine and will
        // happily write 2 with "tar -b2".  With -b0 tar complains
        // about invalid blocking factor but with -b1 it quietly still
        // uses 2.  Or, may be it wants to have an even size mod 1024.
        void write_finish(std::ostream& so, size_t nblocks=2)
        {
            nblocks = std::max(2UL, nblocks);
            for (size_t ind=0; ind<nblocks*512; ++ind) {
                so.put('\0');
            };
        }

        // Begin a file read, return file size or zero on error.
        size_t read_start(std::istream& si)
        {
            clear();
            si.read(head.as_bytes(), 512);
            if (! si) {
                clear();
                return 0;
            }
            return head.size();
        }

        size_t read_data(std::istream& si, char* buf, size_t bufsize)
        {
            size_t remain = head.size() - loc;
            size_t take = std::min(bufsize, remain);
            si.read(buf, take);
            if (!si) {
                take = si.gcount();
            }
            loc += take;
            if (loc == head.size()) {
                read_pad(si);
                clear();
            }
            return take;
        }

        size_t read_file(std::istream& si, char* buf, size_t bufsize)
        {
            size_t got = read_start(si);
            if (got == 0) {
                return 0;
            }
            return read_data(si, buf, bufsize);
        }
        
        void read_pad(std::istream& si) {
            const size_t jump = 512 - loc%512;
            si.seekg(jump, si.cur);
        }

    };

            

}

#endif
