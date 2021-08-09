/**
   A tar reader and writer as boost::iostreams filters.
 */

#ifndef boost_custart_hpp
#define boost_custart_hpp

#include "custard.hpp"

#include <boost/iostreams/concepts.hpp>    // multichar_output_filter
#include <boost/iostreams/operations.hpp> // write

namespace custard {

    // This is a stateful filter which parses the stream for
    // [filename]\n[size-as-string]\n[file body of given size][filename]\n....
    class tar_writer : public boost::iostreams::multichar_output_filter {
      public:

        std::streamsize slurp_filename(const char* s, std::streamsize n)
        {
            for (int ind=0; ind<n; ++ind) {
                if (s[ind] == '\n') {
                    sizebytes="";
                    state = State::size;
                    return ind+1;
                }
                filename += s[ind];
            }
            return n;
        }

        std::streamsize slurp_size(const char* s, std::streamsize n)
        {
            for (int ind=0; ind<n; ++ind) {
                if (s[ind] == '\n') {
                    size = atol(sizebytes.c_str());
                    state = State::header;
                    return ind+1;
                }
                sizebytes += s[ind];
            }
            return n;
        }

        template<typename Sink>
        std::streamsize slurp_body(Sink& dest, const char* s, std::streamsize n)
        {
            std::streamsize remain = th.size() - loc;
            std::streamsize take = std::min(n, remain);
            boost::iostreams::write(dest, s, take);
            loc += take;
            if (loc == th.size()) {
                // pad
                size_t pad = 512 - loc%512;

                for (size_t ind=0; ind<pad; ++ind) {
                    const char zero = '\0';
                    boost::iostreams::write(dest, &zero, 1);
                };

                loc = 0;
                size = 0;
                filename = "";
                th.clear();
                state = State::filename;
            }
            return take;        
        }

    
        template<typename Sink>
        std::streamsize write_one(Sink& dest, const char* s, std::streamsize n)
        {
            if (state == State::filename) {
                return slurp_filename(s, n);
            }

            if (state == State::size) {
                auto ret = slurp_size(s, n);
                if (state == State::header) {
                    loc = 0;
                    th.init(filename, size);
                    boost::iostreams::write(dest, (char*)th.as_bytes(), 512);

                    state = State::body;
                }
                return ret;
            }

            if (state == State::body) {
                return slurp_body(dest, s, n);
            }
            return 0;
        }

        template<typename Sink>
        std::streamsize write(Sink& dest, const char* buf, std::streamsize bufsiz)
        {
            std::streamsize consumed = 0; // number taken from buf
            const char* ptr = buf;
            while (consumed < bufsiz) {
                std::streamsize left = bufsiz - consumed;
                auto took = write_one(dest, ptr, left);
                if (took < 0) {
                    return took;
                }
                if (!took) {
                    return consumed;
                }
                consumed += took;
                ptr += took;
            }
            return bufsiz;
        }

        template<typename Sink>
        void close(Sink& dest) {
            for (size_t ind=0; ind<2*512; ++ind) {
                const char zero = '\0';
                boost::iostreams::write(dest, &zero, 1);
            };
        }
      private:
        custard::Header th;

        std::string filename{""}, sizebytes{""};
        size_t size{0}, loc{0};
        enum class State { filename, size, header, body };
        State state{State::filename};
    };


    // This filter inputs a tar stream and produces a stream matching
    // the output_filter's input expectation:
    // [filename]\n[size-as-string]\n[file body of given size][filename]\n....
    class tar_reader : public boost::iostreams::multichar_input_filter {
      public:

        template<typename Source>
        std::streamsize slurp_header(Source& src)
        {
            auto got = boost::iostreams::read(src, th.as_bytes(), 512);
            if (got < 0) {
                return got;
            }
            assert (got == 512); // fixme: handle short reads

            // drain trailing zeroed "blockings"
            if (th.size() == 0) {
                return slurp_header(src);
            }

            filename = th.name() + "\n";
            bodysize = bodyleft = th.size();
            std::stringstream ss;
            ss << bodyleft << "\n";
            sizebytes = ss.str();
            return 512;
        }

        template<typename Source>
        std::streamsize read_one(Source& src, char* buf, std::streamsize bufsiz)
        {
            if (state == State::header) {
                auto got = slurp_header(src);
                if (got < 0) { return got; }
                state = State::filename;
                return read_one(src, buf, bufsiz);
            }

            // The rest may take several calls if bufsiz is too small.

            if (state == State::filename) { 
                std::streamsize tosend = std::min<size_t>(bufsiz, filename.size());
                std::memcpy(buf, filename.data(), tosend);
                filename.erase(0, tosend);
                if (filename.empty()) {
                    state = State::size;
                }
                return tosend;
            }

            if (state == State::size) {
                std::streamsize tosend = std::min<size_t>(bufsiz, sizebytes.size());
                std::memcpy(buf, sizebytes.data(), tosend);
                sizebytes.erase(0, tosend);
                if (sizebytes.empty()) {
                    state = State::body;
                }
                return tosend;
            }

            if (state == State::body) {
                std::streamsize tosend = std::min<size_t>(bufsiz, bodyleft);
                auto got = boost::iostreams::read(src, buf, tosend);
                if (got < 0) {
                    return got;
                }
                // fixme: handle short read

                bodyleft -= tosend;
                if (bodyleft == 0) {
                    state = State::header;

                    // slurp padding
                    const size_t jump = 512 - bodysize%512;
                    std::string pad(jump, 0);
                    got = boost::iostreams::read(src, &pad[0], jump);
                    if (got < 0) {
                        return got;
                    }
                    // fixme: handle short read
                }
                return tosend;                
            }
            return -1;
        }


        template<typename Source>
        std::streamsize read(Source& src, char* buf, std::streamsize bufsiz)
        {
            std::streamsize filled = 0; // number filled in buf
            char* ptr = buf;
            while (filled < bufsiz) {
                std::streamsize left = bufsiz - filled;
                auto got = read_one(src, ptr, left);
                if (got < 0) {
                    return got;
                }
                if (!got) {
                    return filled;
                }
                filled += got;
                ptr += got;
            }
            return filled;
        }

      private:
        custard::Header th;

        std::string filename{""}, sizebytes{""};
        size_t bodysize{0}, bodyleft{0};
        enum class State { header, filename, size, body };
        State state{State::header};
        
    };

}
#endif
