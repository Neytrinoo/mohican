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

private:
    class ServerSettings *server_settings;
    std::map<int, class ClientConnection> client_connections;
    int listen_sock;
    // std::vector<int> client_sockets;  // vector of file descriptors of the sockets of the clients being processed

    // Network net{};  // Передаем класс Network для установки соединения с  апстримом
};
