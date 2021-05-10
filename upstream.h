#ifndef MOHICAN_UPSTREAM_H
#define MOHICAN_UPSTREAM_H

#include "config_parse.h"
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

class UpstreamServer {
public:
    UpstreamServer() = default;
    ~UpstreamServer() = default;

    int server_start();
    static int daemonize();

    int log_open();
    void all_logs_close();
    static int certain_log_close(std::ofstream stream_to_log);
    int log_message(std::string server_name, log_level_t level, std::string status);

    static int process_setup_signals();  // set handlers to signals
    static void sighup_handler(int sig);  // handler for soft stop
    static void sigint_handler(int sig);  // handler for hard stop
    static void sigpipe_handler(int sig);  // handler for reload

    int server_stop();

    int server_reload();

private:
    std::ofstream stream_to_access_log;
    std::string access_log;
    std::ofstream stream_to_error_log;
    std::string error_log;
    int status_active;

    //TODO: не создавать обьекты классов а спарсить сразу из конфига

    class ServerSettings servers;

    MainServerSettings nginx_settings;  // TODO: реализовать default путь
};

#endif //MOHICAN_UPSTREAM_H
