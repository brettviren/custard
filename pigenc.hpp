/**
   pigenc is an array serialization codec for Numpy format.
 */

#ifndef pigenc_hpp
#define pigenc_hpp

#include <iostream>             // testing
#include <istream>
#include <ostream>
#include <string>
#include <regex>

#include "nlohmann/json.hpp"

namespace pigenc {

    using json = nlohmann::json;

    inline
    json parse_header(std::string header) {

        std::regex q("(')");
        std::regex op("(\\()");
        std::regex cp("(\\))");
        std::regex truify("(True)");
        std::regex falsify("(False)");
        std::regex trailcomma("(, ?)([}\\]])");

        header = std::regex_replace(header, q, "\"");
        header = std::regex_replace(header, op, "[");
        header = std::regex_replace(header, cp, "]");
        header = std::regex_replace(header, truify, "true");
        header = std::regex_replace(header, falsify, "false");
        header = std::regex_replace(header, trailcomma, "$2");

        return json::parse(header);
    }

    inline
    json read_header(std::istream& si) {
        std::string mver(8, 0);
        si.read(&mver[0], 8);
        uint32_t hsize{0};
        if (mver[6] == '\x01') {
            uint16_t s{0};
            si.read((char*)&s, 2);
            hsize = s;
        }
        else {
            si.read((char*)&hsize, 4);
        }

        std::string header(hsize, 0);
        si.read(&header[0], hsize);

        auto ret = parse_header(header);
        ret["version"] = {(int)mver[6], (int)mver[7]};
        return ret;
    }


    std::string make_header(std::string desc, 
                            std::vector<size_t> shape,
                            bool fortran_order=false)
    {
        std::string tf = fortran_order ? "True" : "False";
        std::stringstream ss;
        ss << "{'descr': '" << desc << "', "
           << "'fortran_order': " << tf << ", "
           << "'shape': (";
        for (auto s : shape) {
            ss << s << ", ";
        }
        ss << "), }";
        std::string dict = ss.str();

        int siz = 10 + dict.size();
        int pad = 64 - siz%64;
        uint16_t hsize = pad + siz - 10;

        std::string ret = "\x93NUMPY";
        ret.push_back( '\x01' );
        ret.push_back( '\x00' );
        ret.push_back( *(((char*)&hsize)+0) );
        ret.push_back( *(((char*)&hsize)+1) );
        ret += dict;
        for (int ind=0; ind<pad-1; ++ind) {
            ret.push_back(' ');
        }
        ret.push_back('\n');
        return ret;
    }

    template<typename T> std::string dtype() { return ""; }
    template<> std::string dtype<char>()     { return "c"; }
    template<> std::string dtype<int8_t>()   { return "<i1"; }
    template<> std::string dtype<uint8_t>()  { return "<u1"; }
    template<> std::string dtype<int16_t>()  { return "<i2"; }
    template<> std::string dtype<uint16_t>() { return "<u2"; }
    template<> std::string dtype<int32_t>()  { return "<i4"; }
    template<> std::string dtype<uint32_t>() { return "<u4"; }
    template<> std::string dtype<int64_t>()  { return "<i8"; }
    template<> std::string dtype<uint64_t>() { return "<u8"; }
    template<> std::string dtype<float>()    { return "<f4"; }
    template<> std::string dtype<double>()   { return "<f8"; }

    // This assumes string like "...NN" where NN is number of bytes.
    size_t dtype_size(std::string dt)
    {
        while (dt.size() and (dt[0]-'0' < 0 or dt[0]-'0' > 9)) {
            dt.erase(dt.begin());
        }
        if (dt.empty()) {
            return 0;
        }
        return std::stol(dt);
    }

    // /// Write simple, raw array of elements type Type
    template<typename Type>
    std::ostream& write_data(std::ostream& os,
                             Type* data,
                             std::vector<size_t> shape,
                             bool fortran_order=false)
    {
        auto head = make_header(dtype<Type>(), shape, fortran_order);
        size_t size = 1;
        for (auto s: shape) {
            size *= s;
        }
        os.write(head.data(), head.size());
        os.write((char*)data, size*sizeof(Type));
        return os;
    }
        

    /// Represent the npy header for a simple array (not structured).
    class Header {
        // String rep of Python dictionary describing the array
        std::string descr_{""};

        // Array dimension sizes
        std::vector<size_t> shape_{};

        // storage order
        bool fortran_order_{false};

        // derived values:

        std::string header_string_{""};

      public:

        // Set the header values given C++ element type
        template<typename Type>
        void set(const std::vector<size_t>& shape,
                 bool fortran_order=false)
        {
            shape_ = shape;
            fortran_order_ = fortran_order;
            descr_ = dtype<Type>();
            update();
        }

        // Set the header given descr as string
        void set(const std::vector<size_t>& shape,
                 const std::string& descr,
                 bool fortran_order=false)
        {
            shape_ = shape;
            fortran_order_ = fortran_order;
            descr_ = descr;
            update();
        }

        size_t type_size() const {
            // note, only simple arrays have descr as dtype
            return dtype_size(descr_);
        }

        // Number of elements in the array
        size_t array_size() const {
            size_t size = 1;
            for (auto s: shape_) {
                size *= s;
            }
            return size;
        }
        
        // The size of the array part in bytes.  
        size_t data_size() const {
            return type_size() * array_size();
        }

        size_t header_size() const {
            return str().size();
        }

        // Size of the entire file
        size_t file_size() const {
            return header_size() + data_size();
        }

        // Update derived values
        void update() {
            std::string tf = fortran_order_ ? "True" : "False";
            std::stringstream ss;
            ss << "{'descr': '" << descr_ << "', "
               << "'fortran_order': " << tf << ", "
               << "'shape': (";
            for (auto s : shape_) {
                ss << s << ", ";
            }
            ss << "), }";
            std::string dict = ss.str();

            int siz = 10 + dict.size();
            int pad = 64 - siz%64;
            uint16_t hlen = pad + siz - 10;

            std::string ret = "\x93NUMPY";
            ret.push_back( '\x01' );
            ret.push_back( '\x00' );
            ret.push_back( *(((char*)&hlen)+0) );
            ret.push_back( *(((char*)&hlen)+1) );
            ret += dict;
            for (int ind=0; ind<pad-1; ++ind) {
                ret.push_back(' ');
            }
            ret.push_back('\n');
            header_string_ = ret;
            assert(hlen+10 == str().size());
        }

        std::string str() const {
            return header_string_;
        }

        // Read self from istream
        std::istream& read(std::istream& si)
        {
            auto dat = read_header(si);
            descr_ = dat["descr"];
            fortran_order_ = dat["fortran_order"];
            shape_.clear();
            for (auto s : dat["shape"]) {
                size_t siz = s;
                shape_.push_back(siz);
            }
            update();
            return si;
        }

        std::ostream& write(std::ostream& so)
        {
            so.write(header_string_.data(), header_string_.size());
            return so;
        }

    };

}

#endif

