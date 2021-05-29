#ifndef TEST_MOHICAN_LOG_H
#define TEST_MOHICAN_LOG_H

#include <string>
#include <fstream>
#include <boost/log/sinks.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>

namespace bl = boost::log;
namespace kw = bl::keywords;

using backend_type = bl::sinks::text_file_backend;
using sink_type = bl::sinks::synchronous_sink<backend_type>;
using logger_type = bl::sources::severity_logger<bl::trivial::severity_level>;
static logger_type g_logger;

const std::string g_format = "[%TimeStamp%] [%Severity%] %Message%"; //(%LineID%) [%Uptime%]
BOOST_LOG_ATTRIBUTE_KEYWORD(tag_attr, "Tag", std::string);

class MohicanLog {
public:
    MohicanLog(std::string file, bool flush_flag, bl::trivial::severity_level lvl);
    MohicanLog() = default;

    MohicanLog &operator=(const MohicanLog &other) = default;

    void log(const std::string& s, bl::trivial::severity_level level_message);

private:
    std::string tag_;
    bool key_flush;
    bl::trivial::severity_level log_level;
};


#endif //TEST_MOHICAN_LOG_H
