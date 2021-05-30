#pragma once

#include <unistd.h>
#include <vector>
#include <string>
#include <map>

#include "server_settings.h"
#include "client_connection.h"
#include "mohican_log.h"
#include "define_log.h"

class WorkerProcess {
public:
    explicit WorkerProcess(int listen_sock, class ServerSettings *server_settings, std::vector<MohicanLog*>& vector_logs);

    void run();

    ~WorkerProcess() = default;

    void setup_sighandlers();

    static void sighup_handler(int sig);
    static void sigint_handler(int sig);
    static void sigpoll_handler(int sig);

    void write_to_logs(std::string message, bl::trivial::severity_level lvl);

    bool is_banned_upstream(const std::string& ip, int result_code);

private:
    typedef enum {
        INFO_HARD_STOP_DONE,
        INFO_SOFT_STOP_START,
        INFO_SOFT_STOP_DONE
    } log_messages_t;

    class ServerSettings *server_settings;
    int listen_sock;
    void write_to_log(log_messages_t log_type);

    std::vector<std::string> banned_upstreams;
    
    void message_to_log(log_messages_t log_type);

    std::vector<MohicanLog*> vector_logs;
};
