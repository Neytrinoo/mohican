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
#include <sys/types.h>
#include <sys/wait.h>

typedef boost::log::sources::severity_logger<boost::log::trivial::severity_level> logger_t;

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace attrs = boost::log::attributes;

extern int process_soft_stop;
extern int process_hard_stop;
extern int process_soft_reload;
extern int process_hard_reload;
extern std::string path_to_config;

typedef enum {
    SOFT_LEVEL,
    HARD_LEVEL,
    NULL_LEVEL
} action_level_t;

typedef enum {
    START_SERVER,
    RELOAD_SERVER,
} status_server_action;

class MohicanServer {
public:
    MohicanServer();

    ~MohicanServer() = default;

    int server_start();
        bl::trivial::severity_level cast_types_logs_level(std::string lvl);
        void write_to_logs(std::string message, bl::trivial::severity_level lvl);

        int daemonize(status_server_action server_action);
        bool bind_listen_sock();
        int add_work_processes(status_server_action server_action, action_level_t lvl);
        int fill_pid_file(status_server_action server_action, action_level_t lvl);
        int delete_pid_file();

    static int process_setup_signals();  // set handlers to signals
        static void sighup_handler(int sig);  // handler for soft stop
        static void sigint_handler(int sig);  // handler for hard stop
        static void sigpipe_handler(int sig);  // handler for soft reload
        static void sigalrm_handler(int sig);  // handler for hard reload

    int server_stop(action_level_t level);

    int server_reload(action_level_t level);
        int apply_config(status_server_action server_action, action_level_t level);

private:
    int count_workflows;
    pid_t old_master_process;
    pid_t new_master_process;
    pid_t old_master_process_parent;
    pid_t new_master_process_parent;

    std::vector<pid_t> workers_pid;
    std::vector<pid_t> new_workers_pid;

    std::vector<location_t> upstream_ban_list;  // бан-лист апстримов

    std::string access_log_level = "debug";
    std::string error_log_level = "error";
    std::string access_lvl_before_reload, error_lvl_before_reload;
    MohicanLog error_log = MohicanLog ("error", true, cast_types_logs_level(error_log_level));
    MohicanLog access_log = MohicanLog ("access", true, cast_types_logs_level(access_log_level));

    std::vector<MohicanLog*> vector_logs;
    std::vector<MohicanLog*> new_vector_logs;  // TODO: check why not used, error

    class ServerSettings server;

    int listen_sock;

    MainServerSettings mohican_settings;
    MainServerSettings new_mohican_settings;
};

#endif //NGINX_PROJECT_NGINXSERVER_H
