#include "custard_boost.hpp"
#include <iostream>
#include <iomanip>

/*
  stream = *member
  member = name *option body
  name = "name" SP namestring LF
  body = "body" SP numstring LF data
  data = *OCTET
  option = ("mode" / "mtime" / "uid" / "gid") SP numstring LF
  option /= ("uname" / "gname" ) SP identstring LF

  LF = '\n'
*/

std::pair<bool, std::string> test(std::string input, custard::dictionary_t& table)
{
    std::string::const_iterator f(input.begin());
    std::string::const_iterator l(input.end());

    table.clear();

    bool ok = custard::parse_vars(f,l,table);
    return std::make_pair(ok, std::string(f,l));
}
int main()
{
    custard::dictionary_t table;
    std::string give;
    
    {
        // should fail
        give = "name foo.tar\nbody ";
        auto [ok, left] = test(give, table);
        assert(!ok);
        assert(left == give);
    }
    {
        // should succeed
        give = "name foo.tar\nbody 0\n";
        auto [ok, left] = test(give, table);
        assert(ok);
        assert(table["name"] == "foo.tar");
        assert(table["body"] == "0");
        assert(left == "");
    }
    {
        // should succeed
        give = "name foo.tar\nbody 0\nfoo bar";
        auto [ok, left] = test(give, table);
        assert(ok);
        assert(table["name"] == "foo.tar");
        assert(table["body"] == "0");
        assert(left == "foo bar");
    }
    {
        give = "name foo.tar\nmode 0x777\nbody 5\nhello";
        auto [ok, left] = test(give, table);
        assert(ok);
        assert(left == "hello");
        assert(table["name"] == "foo.tar");
        assert(table["mode"] == "0x777");
        assert(table["body"] == "5");
    }
    {
        give = "name foo.tar\nmode 0x777\nbody 0\nblerg hello\n";
        auto [ok, left] = test(give, table);
        assert(ok);
        assert(table["name"] == "foo.tar");
        assert(table["mode"] == "0x777");
        assert(table["body"] == "0");
        assert(left == "blerg hello\n");
    }
    return 0; 
}
