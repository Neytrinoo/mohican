#include "mohican_log.h"

MohicanLog::MohicanLog(std::string file, bool flush_flag, bl::trivial::severity_level lvl) : tag_(file), key_flush(flush_flag), log_level(lvl) {
    bl::register_simple_formatter_factory<bl::trivial::severity_level, char>("Severity");

    auto backend = boost::make_shared<backend_type>(
            kw::file_name = file + ".log",
            kw::auto_flush = flush_flag);

    auto sink = boost::make_shared<sink_type>(backend);
    sink->set_formatter(bl::parse_formatter(g_format));
    sink->set_filter(tag_attr == tag_);

    boost::log::add_common_attributes();

    bl::core::get()->add_sink(sink);
}

void MohicanLog::log(const std::string& s, bl::trivial::severity_level level_message) {
    if (level_message >= this->log_level) {
        BOOST_LOG_SCOPED_THREAD_TAG("Tag", tag_);
        BOOST_LOG_SEV(g_logger, level_message) << s;
    }
}
