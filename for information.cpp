#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/attributes.hpp>


typedef boost::log::sources::severity_logger<boost::log::trivial::severity_level> logger_t;

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace attrs = boost::log::attributes;

logger_t init_log(const std::string path_to_log, const std::string level_log, bool key) {
    boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");

    logger_t lg;

    boost::log::add_file_log(
            boost::log::keywords::auto_flush = key,
            boost::log::keywords::file_name = path_to_log,
            boost::log::keywords::format = "[%TimeStamp%] [%Severity%] %Message%"
    );
    /*[%Uptime%]*/
    if (level_log != "debug") {
        if (level_log == "info") {
            boost::log::core::get()->set_filter(boost::log::trivial::severity == boost::log::trivial::info);
        } else {
            boost::log::core::get()->set_filter(boost::log::trivial::severity == boost::log::trivial::error);
        }
    }

    boost::log::add_common_attributes();

    logging::core::get()->add_global_attribute("Uptime", attrs::timer());

    return lg;
}

int main() {
    std::string level_log1 = "debug";
    std::string path_to_log1 = "mohican.log";

    init_log(path_to_log1, level_log1, true);

    BOOST_LOG_TRIVIAL(info) << "SERVER STARTED!";
    BOOST_LOG_TRIVIAL(info) << "Worker processes (5) successfully started";
    BOOST_LOG_TRIVIAL(warning) << "SERVER RELOADING...";
    BOOST_LOG_TRIVIAL(info) << "SERVER RELOADED!";
    BOOST_LOG_TRIVIAL(warning) << "SOFT SERVER STOP...";
    BOOST_LOG_TRIVIAL(warning) << "HARD SERVER STOP...";
    BOOST_LOG_TRIVIAL(info) << "SERVER STOPPED!";

    BOOST_LOG_TRIVIAL(info) << "New connection   [URL : " << path_to_log1 << "]   [WORKER PID : " << getpid() << "]";
    BOOST_LOG_TRIVIAL(info) << "New connection   [URL : " << path_to_log1 << "]   [WORKER PID : " << getpid() << "]";

    BOOST_LOG_TRIVIAL(error) << "Connection on WORKER " << getpid() << " faults";

    BOOST_LOG_TRIVIAL(info) << "SERVER STOPPED!";
    BOOST_LOG_TRIVIAL(warning) << "Upstream [SERVERNAME : Local_host] [IP : 192.89.89.89] not respond to request from worker " << getpid();

    BOOST_LOG_TRIVIAL(error) << "Upstream " << "[SERVERNAME : Local_host] [IP : 192.89.89.89]" << " was added to ban-list";
    BOOST_LOG_TRIVIAL(error) << "Upstream " << "[SERVERNAME : Local_host] [IP : 192.89.89.89]" << " didn't respond after timeout";
    BOOST_LOG_TRIVIAL(info) << "Upstream " << "[SERVERNAME : Local_host] [IP : 192.89.89.89]" << " returned work status";
    BOOST_LOG_TRIVIAL(info) << "Upstream " << "[SERVERNAME : Local_host] "
                                              "[IP : 192.89.89.89]" << " returned status = 200 OK on [URL : " << path_to_log1 << "]";


    return 0;
}
