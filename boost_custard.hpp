#ifndef boost_custart_hpp
#define boost_custart_hpp

#include "custard.hpp"

#include <boost/iostreams/concepts.hpp>    // multichar_output_filter
#include <boost/iostreams/operations.hpp> // write

namespace custard {

    // This is a stateful filter which parses the stream for
    // [filename]\n[size-as-string]\n[file body of given size][filename]....
    class output_filter : public boost::iostreams::multichar_output_filter {
      public:

        std::streamsize slurp_filename(const char* s, std::streamsize n)
        {
            for (int ind=0; ind<n; ++ind) {
                if (s[ind] == '\n') {
                    sizebytes="";
                    state = State::size;
                    //std::cerr << "got filename: |" << filename << "|\n";
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
                    //std::cerr << "got size: |" << size << "| from |" << sizebytes << "|\n";
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
            //std::cerr <<  "body " << take << " to " << loc << std::endl;
            if (loc == th.size()) {
                // pad
                size_t pad = 512 - loc%512;
                //std::cerr << "reach size: " << loc << " pad to " << pad << std::endl;

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
        std::streamsize write(Sink& dest, const char* s, std::streamsize nin)
        {
            std::streamsize consumed = 0;
            const char* ptr = s;
            while (consumed < nin) {
                std::streamsize left = nin - consumed;
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
            return nin;
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

}
#endif
