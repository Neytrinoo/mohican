#ifndef NGINX_PROJECT_NGINXSERVER_H
#define NGINX_PROJECT_NGINXSERVER_H

#include "main_server_settings.h"
#include "server_settings.h"
#include "worker_process.h"

#include <vector>
#include <string>
#include <fstream>


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
    MohicanServer() {
        count_workflows = mohican_settings.get_count_workflows();
        server = mohican_settings.get_server();
    }
    ~MohicanServer() = default;

    int server_start();
        static int daemonize();
        bool bind_listen_sock();
        int add_work_processes();
        int fill_pid_file();

        int log_open();
        void all_logs_close();
        static int certain_log_close(std::ofstream stream_to_log);
        int log_message(std::string server_name, log_level_t level, std::string status);
        int start_balancing(int *number_process);

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

    class ServerSettings server;

    int listen_sock;

    MainServerSettings mohican_settings;  // TODO: реализовать default путь
};

#endif //NGINX_PROJECT_NGINXSERVER_H
