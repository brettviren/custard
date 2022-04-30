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

void test(std::string input)
{
    std::cerr << "Parsing:\n\t" << std::quoted(input) << "\n";

    std::string::const_iterator f(input.begin());
    std::string::const_iterator l(input.end());

    custard::dictionary_t table;

    bool ok = custard::parse_vars(f,l,table);
    if (ok) {
        std::cerr << "Success:\n";
        bool found_body = false;
        for (const auto& [k,v] : table) {
            std::cerr << "\t" << std::quoted(k) << " = " << std::quoted(v) << std::endl;
            if (k == "body") {
                found_body = true;
            }
        }
        if (!found_body) {
            std::cerr << "but found no body\n";
        }
    }
    else {
        std::cerr << "Failed\n";
    }
    if (f!=l) {
        std::cerr << "Unparsed: " << std::quoted(std::string(f,l)) << "\n";
    }

}
int main()
{
    test("name foo.tar\nbody ");

    test("name foo.tar\nmode 0x777\nbody 5\nhello");


    return 0; 
}
