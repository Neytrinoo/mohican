#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <csignal>
#include "mohican_server.h"
#include "http_request.h"


int main() {

    MohicanServer server;
    if (server.server_start() != 0) {
        return -1;
    }

    /*
    MainServerSettings server("settings/mohican.conf");
    server.get_server().print_properties();
     */


    /*
    int listen_sock;
    MainServerSettings server("../settings/mohican.conf");
    ServerSettings server_settings = server.get_server();
    //server_settings.print_properties();
    //std::cout << server_settings.get_root("/index.html");

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == -1)
        return -1;

    int enable = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
        return -1;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_settings.get_port());
    if (inet_aton(server_settings.get_servername().c_str(), &addr.sin_addr) == -1)
        return -1;

    if (bind(listen_sock, (struct sockaddr *) &addr, sizeof(addr)) == -1)
        return -1;

    if (listen(listen_sock, 128) == -1) {
        return -1;
    }
    MohicanLog error_log = MohicanLog ("error", true, ERROR);
    MohicanLog access_log = MohicanLog ("access", true, DEBUG);
    std::vector<MohicanLog *> vector_logs;
    vector_logs.push_back(&error_log);
    vector_logs.push_back(&access_log);
    WorkerProcess worker(listen_sock, &server_settings, vector_logs);

    worker.run();



    //MainServerSettings config_serv("../config/src/mohican.conf");
    //config_serv.get_server().print_properties();
    */

    return 0;
}
