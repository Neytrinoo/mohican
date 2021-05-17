#include <sys/socket.h>
#include <sys/epoll.h>
#include <iostream>
#include <fcntl.h>
#include <csignal>


#include "worker_process.h"
#include "server_settings.h"
#include "client_connection.h"

#define EPOLL_SIZE 1024
#define EPOLL_RUN_TIMEOUT -1
#define MAX_METHOD_LENGTH 4
#define CLIENT_SEC_TIMEOUT 5 // maximum request idle time

WorkerProcess::WorkerProcess(int listen_sock, class ServerSettings *server_settings) : listen_sock(listen_sock),
                                                                                       server_settings(
                                                                                               server_settings) {
    signal(SIGPIPE, SIG_IGN);
}

void WorkerProcess::run() {
    static struct epoll_event ev, events[EPOLL_SIZE];
    ev.events = EPOLLIN | EPOLLET;

    int epoll_fd = epoll_create(EPOLL_SIZE);
    ev.data.fd = this->listen_sock;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, this->listen_sock, &ev);

    int client, epoll_events_count;

    while (true) {
        epoll_events_count = epoll_wait(epoll_fd, events, EPOLL_SIZE, EPOLL_RUN_TIMEOUT);
        for (int i = 0; i < epoll_events_count; ++i) {
            if (events[i].data.fd == this->listen_sock) {
                client = accept(this->listen_sock, NULL, NULL);
                fcntl(client, F_SETFL, fcntl(client, F_GETFL, 0) | O_NONBLOCK);
                ev.data.fd = client;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client, &ev);
                this->client_connections[client] = ClientConnection(client, this->server_settings);
                std::cout << "connection send, client sock = " << client << std::endl;
            } else {  // if the event happened on a client socket
                connection_status_t connection_status = this->client_connections[events[i].data.fd].connection_processing();
                if (connection_status == CONNECTION_FINISHED || connection_status == CONNECTION_TIMEOUT_ERROR ||
                    connection_status == ERROR_WHILE_CONNECTION_PROCESSING) {
                    std::cout << "something wrong, client sock = " << events[i].data.fd << std::endl;
                    this->client_connections.erase(events[i].data.fd);
                    close(events[i].data.fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
                }
            }
        }
    }
}

