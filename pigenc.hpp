/**
   pigenc is an array serialization codec for Numpy format.
 */

#ifndef pigenc_hpp
#define pigenc_hpp

#include <iostream>
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
        std::string comma="";
        for (auto s : shape) {
            ss << comma << s;
            comma = ", ";
        }
        ss << ")}\n";
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
        for (int ind=0; ind<pad; ++ind) {
            ret.push_back(' ');
        }
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
        

    // template<typename Array>
    // std::ostream& write(std::ostream& so, const Array& array)
    // {
    // }

}

#endif
