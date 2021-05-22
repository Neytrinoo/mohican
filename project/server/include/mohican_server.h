#ifndef NGINX_PROJECT_NGINXSERVER_H
#define NGINX_PROJECT_NGINXSERVER_H

#include "main_server_settings.h"
#include "server_settings.h"
#include "worker_process.h"
#include "mohican_log.h"
#include "define_log.h"

#include <vector>
#include <string>
#include <fstream>
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

extern int process_soft_stop;
extern int process_hard_stop;
extern int process_reload;
extern std::string path_to_config;

typedef enum {
    ACCESS_LEVEL,
    ERROR_LEVEL,
} log_level_t;

typedef enum {
    SOFT_LEVEL,
    HARD_LEVEL,
} stop_level_t;

class MohicanServer {
public:
    MohicanServer();

    ~MohicanServer() = default;

    int server_start();
        void init_logs(bool flush_flag);
            bl::trivial::severity_level cast_types_logs_level(std::string lvl);
            void write_to_logs(std::string message, bl::trivial::severity_level lvl);

        static int daemonize();
        bool bind_listen_sock();
        int add_work_processes();
        int fill_pid_file();

    static int process_setup_signals();  // set handlers to signals
        static void sighup_handler(int sig);  // handler for soft stop
        static void sigint_handler(int sig);  // handler for hard stop
        static void sigpipe_handler(int sig);  // handler for reload

    int server_stop(stop_level_t level);

    int server_reload();
        int apply_config();

private:
    int count_workflows;
    std::vector<pid_t> workers_pid;  // list of pid processes to add to pid_file
    std::ofstream stream_to_access_log;
    std::string access_log;
    std::ofstream stream_to_error_log;
    std::string error_log;
    std::vector<location_t> upstream_ban_list;  // бан-лист апстримов

    std::vector<MohicanLog> vector_logs;
    std::string access_log_level, access_lvl_before_reload;  // TODO: from config
    std::string error_log_level, error_lvl_before_reload;  // TODO: in what class it will be

    class ServerSettings server;

    int listen_sock;

    MainServerSettings mohican_settings;  // TODO: реализовать default путь
};

#endif //NGINX_PROJECT_NGINXSERVER_H
