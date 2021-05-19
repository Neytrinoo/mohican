#pragma once

#include <unistd.h>
#include <vector>
#include <string>
#include <map>

#include "server_settings.h"
#include "client_connection.h"


class WorkerProcess {
public:
    explicit WorkerProcess(int listen_sock, class ServerSettings *server_settings);

    void run();

    ~WorkerProcess() = default;

    void setup_sighandlers();

    static void sighup_handler(int sig);
    static void sigint_handler(int sig);

private:
    typedef enum {
        INFO_HARD_STOP_DONE,
        INFO_SOFT_STOP_START,
        INFO_SOFT_STOP_DONE
    } log_messages_t;

    class ServerSettings *server_settings;
    std::map<int, class ClientConnection> client_connections;
    int listen_sock;
    void write_to_log(log_messages_t log_type);
};
