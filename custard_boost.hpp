/**
   custard filters

   Provide boost::iostreams input and output filters to convert
   between custard and tar codec.
 */

#ifndef boost_custart_hpp
#define boost_custart_hpp

#include "custard.hpp"
#include "custard_stream.hpp"

#include <boost/iostreams/concepts.hpp>    // multichar_output_filter
#include <boost/iostreams/operations.hpp> // write
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/bzip2.hpp>

#include <boost/filesystem.hpp>

// #include <iostream> // debug

namespace custard {

    inline
    bool assuredir(const std::string& pathname)
    {
        boost::filesystem::path p(pathname);
        if ( ! p.extension().empty() ) {
            p = p.parent_path();
        }
        if (p.empty()) {
            return false;
        }
        return boost::filesystem::create_directories(p);
    }

    // This filter inputs a custard stream and outputs a tar stream.
    class tar_writer : public boost::iostreams::multichar_output_filter {
      public:

        std::streamsize slurp_header(const char* s, std::streamsize nin)
        {
            std::streamsize ind = 0;

            for (; ind<nin; ++ind) {
                char c = s[ind];
                ++ind;
                headstring.push_back(c);

                if (c == '\n') { // just finished a value
                    have_key = false;
                    key_start = headstring.size();
                    if (in_body) {
                        std::istringstream ss(headstring);
                        read(ss, th);
                        loc = 0;
                        headstring.clear();
                        key_start = 0;
                        have_key = false;
                        in_body = false;
                        state = State::sendhead;
                        return ind;
                    }
                    continue;
                }

                if (c == ' ') { 
                    if (have_key) { 
                        // just a random space in a value
                        continue;
                    }
                    // we've just past the key
                    have_key = true;
                    in_body = false;
                    if (headstring.substr(key_start) == "body ") {
                        in_body = true;
                    }
                    continue;
                }
            }

            return nin;
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
                    boost::iostreams::put(dest, '\0');
                };

                loc = 0;
                th.clear();
                state = State::gethead;
            }
            return take;        
        }

    
        template<typename Sink>
        std::streamsize write_one(Sink& dest, const char* s, std::streamsize n)
        {
            if (state == State::gethead) {
                return slurp_header(s, n);
            }

            if (state == State::sendhead) {
                boost::iostreams::write(dest, (char*)th.as_bytes(), 512);
                state = State::body;
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

        // we slurp in the custard header string fully first
        std::string headstring;
        // the start of the current key in the headstring
        size_t key_start{0};
        // we have a seen a key in the current option
        bool have_key{false};
        // If we are in the middle of inputing the "body" key
        bool in_body{false};
        // where in the body we are
        size_t loc{0};
        enum class State { gethead, sendhead, body };
        State state{State::gethead};

    };


    // This filter inputs a tar stream and produces custard stream.
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

            bodyleft = th.size();
            std::stringstream ss;
            custard::read(ss, th);
            headstring = ss.str();


            // drain trailing zeroed "blockings"
            if (th.size() == 0) {
                return slurp_header(src);
            }
            return 512;
        }

        template<typename Source>
        std::streamsize read_one(Source& src, char* buf, std::streamsize bufsiz)
        {
            if (state == State::gethead) {
                auto got = slurp_header(src);
                if (got < 0) { return got; }
                state = State::sendhead;
                return read_one(src, buf, bufsiz);
            }

            // The rest may take several calls if bufsiz is too small.

            if (state == State::sendhead) { 
                std::streamsize tosend = std::min<size_t>(bufsiz, headstring.size());
                std::memcpy(buf, headstring.data(), tosend);
                headstring = headstring.substr(tosend);
                if (headstring.empty()) {
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
                    state = State::gethead;

                    // slurp padding
                    const size_t jump = 512 - th.size()%512;
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
        // temp buffer for custard codec of header
        std::string headstring{""};
        // track how much of the body is left to read
        size_t bodyleft{0};
        enum class State { gethead, sendhead, body };
        State state{State::gethead};
        
    };

    inline
    void input_filters(boost::iostreams::filtering_istream& in,
                       std::string inname)
    {
        if (boost::algorithm::iends_with(inname, ".tar")) {
            in.push(custard::tar_reader());
        }
        else if (boost::algorithm::iends_with(inname, ".tar.gz")) {
            in.push(custard::tar_reader());
        }
        else if (boost::algorithm::iends_with(inname, ".tar.bz2")) {
            in.push(custard::tar_reader());
        }

        if (boost::algorithm::iends_with(inname, ".gz")) {
            in.push(boost::iostreams::gzip_decompressor());
        }
        else if (boost::algorithm::iends_with(inname, ".bz2")) {
            in.push(boost::iostreams::bzip2_decompressor());
        }

        in.push(boost::iostreams::file_source(inname));
    }

    /// Parse outname and based complete the filter ostream.  If
    /// parsing fails, nothing is added to "out".
    inline
    void output_filters(boost::iostreams::filtering_ostream& out,
                        std::string outname,
                        int level = boost::iostreams::zlib::default_compression)
    {
        /// future intentions:
        // if (boost::algorithm::iends_with(outname, "/")) {
        //     out.push(custard::dir_writer(outname));
        //     return;
        // }
        // if (boost::algorithm::istarts_with(outname, "inproc://")) {
        //     out.push(custard::zmq_writer(outname));
        //     return;
        // }

        
        assuredir(outname);

        // Add tar writer if we see tar at the end.
        if (boost::algorithm::iends_with(outname, ".tar")) {
            out.push(custard::tar_writer());
        }
        else if (boost::algorithm::iends_with(outname, ".tar.gz")) {
            out.push(custard::tar_writer());
        }
        else if (boost::algorithm::iends_with(outname, ".tar.bz2")) {
            out.push(custard::tar_writer());
        }

        // In addition, if compression is wanted, add the appropriate
        // filter next.

        if (boost::algorithm::iends_with(outname, ".gz")) {
            out.push(boost::iostreams::gzip_compressor(level));
        }
        else if (boost::algorithm::iends_with(outname, ".bz2")) {
            out.push(boost::iostreams::bzip2_compressor());
        }

        // finally, we save to the actual file.
        out.push(boost::iostreams::file_sink(outname));
    }
}
#endif
