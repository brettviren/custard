/**
   custard streams

   Provide functions to read/write custard header from/to std::stream
   or derived.

   stream = *member
   member = name *option body
   name = "name" SP namestring LF
   body = "body" SP numstring LF data
   data = *OCTET
   option = ("mode" / "mtime" / "uid" / "gid") SP numstring LF
   option /= ("uname" / "gname" ) SP identstring LF

 */
#ifndef custard_stream_hpp
#define custard_stream_hpp

#include "custard.hpp"

namespace custard {

    std::string get_string(std::istream& si, char delim=' ')
    {
        std::string s;
        while (true) {
            std::getline(si, s, delim);
            if (!si) return ""; 
            if (s.empty()) {
                continue;
            }
            break;
        }
        return s;
    }

    size_t get_number(std::istream& si)
    {
        std::string nstr = get_string(si, '\n');
        if (!si) return 0;
        return std::stol(nstr);
    }

    // Read header from stream.  Extract characters from istream
    // filling head until reaching the start of the member body data.
    // Caller MUST continue to read exactly the number of bytes
    // indicated by head.size() in order to position stream at the
    // start of the next archive member.
    std::istream& read(std::istream& si, Header& head)
    {
        while (true) {
            std::string key = get_string(si, ' ');
            if (!si) return si;

            if (key == "name") {
                head.set_name(get_string(si, '\n'));
            }
            else if (key == "mode") {
                head.set_mode(get_number(si));
            }
            else if (key == "mtime") {
                head.set_mtime(get_number(si));
            }
            else if (key == "uid") {
                head.set_uid(get_number(si));
            }
            else if (key == "git") {
                head.set_gid(get_number(si));
            }
            else if (key == "uname") {
                head.set_uname(get_string(si, '\n'));
            }
            else if (key == "gname") {
                head.set_gname(get_string(si, '\n'));
            }
            else if (key == "body") {
                head.set_size(get_number(si));
                head.set_chksum(head.checksum());
                return si;
            }
            if (!si) {
                return si;
            }
            break;
        }
        return si;        
    }
    // The minimum header read.
    std::istream& read(std::istream& si,
                      std::string& filename, size_t& filesize)
    {
        custard::Header head;
        read(si, head);
        if (!si) return si;
        filename = head.name();
        filesize = head.size();
        return si;
    }



    // Stream header to ostream, leaving stream ready to write member
    // body data.  The caller MUST write additional number of bytes as
    // indicated by head.size() in order to avoid a corrupted stream.
    std::ostream& write(std::ostream& so, const Header& head)
    {
        so << "name " << head.name() << "\n"
           << "mode " << head.mode() << "\n"
           << "mtime " << head.mtime() << "\n"
           << "uid " << head.uid() << "\n"
           << "gid " << head.gid() << "\n"
           << "uname " << head.uname() << "\n"
           << "gname " << head.gname() << "\n"
           << "body " << head.size() << "\n";

        return so;
    }

    // The minimum header write:
    std::ostream& write(std::ostream& so,
                        const std::string& filename, size_t filesize)
    {
        so << "name " << filename << "\n"
           << "body " << filesize << "\n";
        return so;
    }

}

#endif
