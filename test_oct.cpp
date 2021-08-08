#include <iostream>
#include <sstream>
#include <ctime>
int main()
{

    int64_t n = 512*0xFF - 1;
    std::stringstream ss;
    ss << std::oct << n << " ";

    time_t t = time(0);
    ss << std::oct << t;
    std::cerr << ss.str() << std::endl;

    return 0;
}
