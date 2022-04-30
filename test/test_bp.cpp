#include <boost/process.hpp>
#include <iostream>

namespace bp = boost::process;

int main()
{
    std::string cmd{"pixz"};
    std::vector<std::string> args{"-p", "1", "-1", "-o", "test_bp.pixz"};
    
    bp::opstream in;
    bp::child child(bp::search_path(cmd), bp::args(args),
                    bp::std_out > stdout, bp::std_err > stderr,
                    bp::std_in < in);
    std::cerr << "made baby\n";
    if (!child.running()) {
        std::cerr << "dead baby\n";
        return 1;
    }
    std::cerr << "sending data\n";
    in.write("hello\n", 6);
    //in << "some data\n";
    //in << std::flush;
    in.close();
    in.pipe().close();
    if (!child.running()) {
        std::cerr << "dead baby\n";
        return 1;
    }
    if (!in) {
        std::cerr << "pipe error\n";
    }
    // std::cerr << "killing baby\n";
    // child.terminate();
    std::cerr << "waiting\n";
    child.wait();
    if (!child.running()) {
        std::cerr << "dead baby\n";
        return 1;
    }

    return child.exit_code();
}
