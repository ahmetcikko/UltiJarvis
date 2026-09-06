#include "platform.h"

#include <boost/version.hpp>
#if BOOST_VERSION >= 108600
#include <boost/process/v1.hpp>
#else
#include <boost/process.hpp>
#endif

#include <string>
#include <thread>
#include <vector>

#if BOOST_VERSION >= 108600
namespace bp = boost::process::v1;
#else
namespace bp = boost::process;
#endif

namespace platform {

bool spawn(const QStringList &args) {
    if (args.isEmpty())
        return false;
    std::vector<std::string> argv;
    for (const QString &a : args)
        argv.push_back(a.toStdString());
    boost::filesystem::path exe;
    try {
        exe = bp::search_path(argv[0]);
    } catch (...) {
    }
    if (exe.empty())
        exe = boost::filesystem::path(argv[0]);
    std::vector<std::string> rest(argv.begin() + 1, argv.end());
    std::string dir = home_dir();
    std::thread([exe, rest, dir]() {
        try {
            if (dir.empty()) {
                bp::child c(exe, rest);
                c.wait();
            } else {
                bp::child c(exe, rest, bp::start_dir(dir));
                c.wait();
            }
        } catch (...) {
        }
    }).detach();
    return true;
}

} // namespace platform
