/**
   custard filters

   Provide boost::iostreams input and output filters to convert
   between custard and tar codec.
 */

#ifndef custard_boost_hpp
#define custard_boost_hpp

#include "custard.hpp"
#include "custard_stream.hpp"

#define MINIZ_NO_ZLIB_APIS
#include "miniz.hpp"

#include <boost/iostreams/concepts.hpp>    // multichar_output_filter
#include <boost/iostreams/operations.hpp> // write
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filter/lzma.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/bzip2.hpp>

#include <boost/filesystem.hpp>
#include <boost/process.hpp>

// #define BOOST_SPIRIT_DEBUG
#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/adapted/std_pair.hpp>

#include <map>
#include <vector>

#include <regex>
#include <sstream>

#include <iostream> // debug

namespace custard {

    namespace qi = boost::spirit::qi;

    using dictionary_t = std::map<std::string, std::string>;

    using buffer_t = std::vector<char>;

    template <typename Iterator>
    struct parser : qi::grammar<Iterator, dictionary_t()>
    {
        using keyval_t = std::pair<std::string, std::string>;

        parser() : parser::base_type(header)
        {
            header = qi::no_skip[ ini >> *opt >> fin ];
            ini = qi::no_skip[qi::string("name") >> ' ' >> value >> '\n'];
            opt = qi::no_skip[keyword >> ' ' >> value >> '\n'];
            fin = qi::no_skip[qi::string("body") >> ' ' >> value >> '\n'];
            keyword = qi::no_skip[ qi::string("mode") | qi::string("mtime") | qi::string("uid") | qi::string("gid") | qi::string("uname") | qi::string("gname") ];
            value = qi::no_skip[+~qi::lit('\n')];

            BOOST_SPIRIT_DEBUG_NODE(header);
            BOOST_SPIRIT_DEBUG_NODE(ini);
            BOOST_SPIRIT_DEBUG_NODE(opt);
            BOOST_SPIRIT_DEBUG_NODE(fin);
            BOOST_SPIRIT_DEBUG_NODE(keyword);
            BOOST_SPIRIT_DEBUG_NODE(value);
        }

      private:
        qi::rule<Iterator, dictionary_t()> header;
        qi::rule<Iterator, keyval_t()> ini, opt, fin;
        qi::rule<Iterator, std::string()> keyword, value;
    };    

    template <typename Iterator>
    bool parse_vars(Iterator &first, Iterator last, dictionary_t & vars)
    {
        parser<Iterator> p;
        return boost::spirit::qi::parse(first, last, p, vars);
    }

    // Base class for a member-at-a-time sink.
    class member_sink {
      public:
        typedef char char_type;
        struct category :
            public boost::iostreams::sink_tag,
            public boost::iostreams::closable_tag
        { };

        virtual ~member_sink() {}

        bool has(const std::string& key) {
            return params.find(key) != params.end();
        }

        int ival(const std::string& key) {
            return atoi(params[key].c_str());
        }
        size_t lval(const std::string& key) {
            return atol(params[key].c_str());
        }
        std::string sval(const std::string& key) {
            return params[key];
        }

        virtual void close_sink() = 0;
        virtual void write_member() = 0;

        // return true if more on the plate, false if starved
        bool write_one()
        {
            size_t buf_size = buf.size();

            if (!buf_size) {
                return false;
            }

            // std::cerr << "write: one: state=" << (int)state
            //           << " [" << buf.size() << "]\n";

            if (state == State::inhead) {
                buffer_t::const_iterator beg(buf.begin());
                buffer_t::const_iterator end(buf.end());
                bool ok = custard::parse_vars(beg, end, params);
                if (!ok) {
                    params.clear();
                    return false;
                }
                buf = buffer_t(beg, end); // more for later
                th.init(sval("name"), lval("body"));
                if (has("mode")) th.set_mode(ival("mode"));
                if (has("mtime")) th.set_mtime(lval("mtime"));
                if (has("uid")) th.set_uid(lval("uid"));
                if (has("gid")) th.set_gid(lval("gid"));
                if (has("uname")) th.set_uname(sval("uname"));
                if (has("gname")) th.set_gname(sval("gname"));
                // std::cerr << "write: header:" << th.name()
                //           << " size=" << th.size()
                //           << " buf:[" << buf.size() << "]\n";
                state = State::inbody;
                // std::cerr << "write: going to state=" << (int)state
                //           << " [" << buf.size() << "]\n";
                return true;
            }

            if (state == State::inbody) {
                if (buf.size() < th.size()) {
                    return false; // keep slurping body
                }
                buffer_t next;
                if (th.size() < buf.size()) {
                    // save left-overs
                    next = buffer_t(buf.begin()+th.size(), buf.end());
                    // truncate to member size
                    buf.resize(th.size());
                }
                write_member();
                clear();
                buf = next;
                state = State::inhead;
                // std::cerr << "write: going to state=" << (int)state
                //           << " [" << buf.size() << "]\n";
                return true;
            }

            return false;
        }
        
        void clear()
        {
            th.clear();
            buf.clear();
            params.clear();
        }

        // return how much we ate
        std::streamsize write(const char* s, std::streamsize n)
        {
            // std::cerr << "write: state=" << (int)state
            //           << " given " << n << " held [" << buf.size() << "]\n";

            // always slurp everything
            buf.insert(buf.end(), s, s+n);

            while (true) {
                bool want_more = write_one();
                if (want_more) {
                    // std::cerr << "write: state=" << (int)state
                    //           << " continuing with [" << buf.size() << "]\n";
                    continue;
                }
                break;
            }
            // std::cerr << "write: state=" << (int)state
            //           << " returning with [" << buf.size() << "]\n";

            return n;
        }

        void close()
        {
            if (buf.size()) {   // last file
                // std::cerr << "write: state=" << (int)state
                //           << " closing with [" << buf.size() << "]\n";
                write_member();
                clear();
                state = State::inhead;
            }
            close_sink();
        }

      protected:
        buffer_t buf;
        Header th;

      private:
        enum class State { inhead, inbody };
        State state{State::inhead};
        dictionary_t params;
    };

    
    class miniz_sink : public member_sink {
      public:
        
        miniz_sink(const std::string& outname)
        {
            // filters need to be copyable
            zip = std::make_shared<mz_zip_archive>();
            memset(zip.get(), 0, sizeof(mz_zip_archive));
            if (!mz_zip_writer_init_file(zip.get(), outname.c_str(), 0)) {
                throw std::runtime_error("failed to initialize miniz for " + outname);
            }
        }
        virtual ~miniz_sink() {}

        void close_sink()
        {
            mz_zip_writer_finalize_archive(zip.get());
            mz_zip_writer_end(zip.get());
            zip = nullptr;
        }

        virtual void write_member()
        {
            auto ok = mz_zip_writer_add_mem(
                zip.get(), th.name().c_str(), buf.data(), buf.size(),
                MZ_BEST_SPEED);
            if (!ok) {
                throw std::runtime_error("failed to write " + th.name());
            }
        }

      private:
        // this class must be copyable
        std::shared_ptr<mz_zip_archive> zip;
    };


    class proc_sink {
      public:
        typedef char char_type;
        struct category :
            public boost::iostreams::sink_tag,
            public boost::iostreams::closable_tag
        { };
        proc_sink(const std::string& cmd, const std::vector<std::string>& args)
        {
            p = std::make_shared<impl>(cmd, args);
        }
        void close()
        {
            p->in.close();
            p->in.pipe().close();
            p->child.wait();
        }
        std::streamsize write(const char* s, std::streamsize n)
        {
            if (!p->child.running()) {
                return 0;
            }
            if (!p->in) {
                return 0;
            }
            p->in.write(s, n);
            if (!p->in) {
                return 0;
            }
            return n;
        }
      private:
        struct impl {
            impl(const std::string& cmd, const std::vector<std::string>& args)
                : in{}, child(boost::process::search_path(cmd),
                              boost::process::args(args),
                              boost::process::std_out > stdout,
                              boost::process::std_err > stderr,
                              boost::process::std_in < in)
            {
            }
            boost::process::opstream in;
            boost::process::child child;
        };
        std::shared_ptr<impl> p;
    };

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
            for (std::streamsize ind = 0; ind<nin; ++ind) {
                char c = s[ind];
                headstring.push_back(c);

                if (c == '\n') { // just finished a value
                    have_key = false;
                    key_start = headstring.size(); // one past the newline
                    if (in_body) {
                        std::istringstream ss(headstring);
                        read(ss, th);
                        loc = 0;
                        headstring.clear();
                        key_start = 0;
                        in_body = false;
                        state = State::sendhead;
                        return ind+1;
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
                    std::string key = headstring.substr(key_start);
                    // std::cerr << "custard write filter: found key: |" << key << "| at " << key_start 
                    //           << " headstring: |" << headstring << "|\n";

                    if (key == "body ") {
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
            assert(take >= 0 and take <= n);
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
                // std::cerr << "custard write filter: going to body, headstring: |" << headstring << "|\n";
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
            // std::cerr << "tar_reader: slurp_header\n";
            std::streamsize got{0};

            // Either we have a next non-zero header or we have zero
            // headers until we hit EOF.
            while (true) {
                got = boost::iostreams::read(src, th.as_bytes(), 512);
                if (got < 0) {
                    // std::cerr << "tar_reader: EOF in header\n";
                    return got;
                }
                if (th.size() == 0) {
                    // std::cerr << "tar_reader: empty header\n";
                    continue;
                }
                break;
            }
            assert (got == 512);

            // std::cerr << "tar_reader: |" << th.name() << "| " << th.size() <<"|\n";

            bodyleft = th.size();
            std::ostringstream ss;
            custard::write(ss, th);
            headstring = ss.str();

            // std::cerr << "\theadstring:\n|" << headstring << "|\n";

            return 512;
        }

        template<typename Source>
        std::streamsize read_one(Source& src, char* buf, std::streamsize bufsiz)
        {
            // std::cerr << "tar_reader: read_one at state "
            //           << (int)state << " bufsize "<<bufsiz<<"\n";
            if (state == State::gethead) {
                auto got = slurp_header(src);
                if (got < 0) {
                    // std::cerr << "tar_reader: forwarding EOF in read_one\n";
                    return got;
                }
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
                std::streamsize want = std::min<size_t>(bufsiz, bodyleft);
                auto put = boost::iostreams::read(src, buf, want);

                // std::cerr << "bodyleft: " << bodyleft << " " << bufsiz
                //           << " " << want << " " << put << "\n";
                if (put < 0) {
                    return put;
                }

                bodyleft -= put;
                if (bodyleft == 0) {
                    state = State::filedone;
                    // slurp padding
                    const size_t jump = 512 - th.size()%512;
                    std::string pad(jump, 0);
                    auto got = boost::iostreams::read(src, &pad[0], jump);
                    // std::cerr << "padleft: " << jump << " " << got << "\n";
                    if (got < 0) {
                        return got;
                    }
                }
                return put;
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
                if (state == State::filedone) {
                    state = State::gethead;
                    return filled;
                }
            }
            return filled;
        }

      private:
        custard::Header th;
        // temp buffer for custard codec of header
        std::string headstring{""};
        // track how much of the body is left to read
        size_t bodyleft{0};
        enum class State { gethead, sendhead, body, getpadd, filedone };
        State state{State::gethead};
        
    };

    // Build input filtering stream based on parsing the filename.
    inline
    void input_filters(boost::iostreams::filtering_istream& in,
                       std::string inname)
    {
        auto has = [&](const std::string& things) {
            return std::regex_search(inname, std::regex("[_.]("+things+")\\b"));
        };

        if (has("tar|tar.gz|tgz|tar.bz2|tbz|tbz2|tar.xz|txz|tar.pixz|tix|tpxz")) {
            in.push(custard::tar_reader());
        }

        if (has("gz|tgz")) {
            in.push(boost::iostreams::gzip_decompressor());
        }
        else if (has("bz2|tbz|tbz2")) {
            in.push(boost::iostreams::bzip2_decompressor());
        }
        else if (has("xz|txz|pixz|tix|tpxz")) {
            // pixz makes xz-compatible files
            in.push(boost::iostreams::lzma_decompressor());
        }

        in.push(boost::iostreams::file_source(inname));
    }

    /// Parse outname and based complete the filter ostream.  If
    /// parsing fails, nothing is added to "out".  The compression
    /// level is interpreted as being from 0-9, higher is more,
    /// slower.
    inline
    void output_filters(boost::iostreams::filtering_ostream& out,
                        std::string outname,
                        int level = 1)
    {
        auto has = [&](const std::string& things) {
            return std::regex_search(outname, std::regex("[_.]("+things+")\\b"));
        };

        assuredir(outname);

        // Add tar writer if we see tar at the end.
        if (has("tar|tar.gz|tgz|tar.bz2|tbz|tbz2|tar.xz|txz|tar.pixz|tix|tpxz")) {
            out.push(custard::tar_writer());
        }

        // In addition, if compression is wanted, add the appropriate
        // filter next.
        if (has("gz|tgz")) {
            out.push(boost::iostreams::gzip_compressor(level));
        }
        else if (has("bz2|tbz|tbz2")) {
            out.push(boost::iostreams::bzip2_compressor(level));
        }
        else if (has("xz|txz")) {
            out.push(boost::iostreams::lzma_compressor(level));
        }
        else if (has("pixz|tix|tpxz")) {
            // std::stringstream ss;
            // ss << "-p 1 -" << level << " -o " << outname;
            // std::cerr << ss.str() << std::endl;
            out.push(custard::proc_sink("pixz", {"-p", "1", "-1", "-o", outname}));
            return;
        }
        else if (has("zip|npz")) {
            out.push(custard::miniz_sink(outname));
            return;
        }
        // finally, we save to the actual file
        out.push(boost::iostreams::file_sink(outname));
    }
}
#endif
