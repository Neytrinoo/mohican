#include <sys/socket.h>
#include <sys/epoll.h>
#include <iostream>
#include <ctime>
#include <fcntl.h>
#include <csignal>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/sources/global_logger_storage.hpp>

#include "worker_process.h"
#include "server_settings.h"
#include "client_connection.h"

#define EPOLL_SIZE 1024
#define EPOLL_RUN_TIMEOUT -1
#define MAX_METHOD_LENGTH 4
#define CLIENT_SEC_TIMEOUT 5 // maximum request idle time

extern bool is_hard_stop = false;
extern bool is_soft_stop = false;

WorkerProcess::WorkerProcess(int listen_sock, class ServerSettings *server_settings) : listen_sock(listen_sock),
                                                                                       server_settings(
                                                                                               server_settings) {
    signal(SIGPIPE, SIG_IGN);
    this->setup_sighandlers();
}

void WorkerProcess::run() {
    static struct epoll_event ev, events[EPOLL_SIZE];
    ev.events = EPOLLIN | EPOLLET;

    int epoll_fd = epoll_create(EPOLL_SIZE);
    ev.data.fd = this->listen_sock;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, this->listen_sock, &ev);

    int client, epoll_events_count;

    while (!is_hard_stop && !is_soft_stop) {
        epoll_events_count = epoll_wait(epoll_fd, events, EPOLL_SIZE, EPOLL_RUN_TIMEOUT);
        for (int i = 0; i < epoll_events_count; ++i) {
            if (events[i].data.fd == this->listen_sock) {
                client = accept(this->listen_sock, NULL, NULL);
                fcntl(client, F_SETFL, fcntl(client, F_GETFL, 0) | O_NONBLOCK);
                ev.data.fd = client;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client, &ev);
                this->client_connections[client] = ClientConnection(client, this->server_settings);
            } else {  // if the event happened on a client socket
                connection_status_t connection_status = this->client_connections[events[i].data.fd].connection_processing();
                if (connection_status == CONNECTION_FINISHED || connection_status == CONNECTION_TIMEOUT_ERROR ||
                    connection_status == ERROR_WHILE_CONNECTION_PROCESSING) {
                    this->client_connections.erase(events[i].data.fd);
                    close(events[i].data.fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
                }
            }
        }
    }

    ev.data.fd = this->listen_sock;
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, this->listen_sock, &ev);

    if (is_soft_stop) {
        this->write_to_log(INFO_SOFT_STOP_START);
        for (int i = 0; i < epoll_events_count; ++i) {
            connection_status_t connection_status = this->client_connections[events[i].data.fd].connection_processing();
            if (connection_status == CONNECTION_FINISHED || connection_status == CONNECTION_TIMEOUT_ERROR ||
                connection_status == ERROR_WHILE_CONNECTION_PROCESSING) {
                this->client_connections.erase(events[i].data.fd);
                close(events[i].data.fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
            }
        }
        this->write_to_log(INFO_SOFT_STOP_DONE);
    } else {
        this->write_to_log(INFO_HARD_STOP_DONE);
    }
    exit(0);
}

void WorkerProcess::sighup_handler(int sig) {
    is_soft_stop = true;
}

void WorkerProcess::sigint_handler(int sig) {
    is_hard_stop = true;
}

void WorkerProcess::setup_sighandlers() {
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = sighup_handler;
    sigaction(SIGHUP, &act, nullptr);
    act.sa_handler = sigint_handler;
    sigaction(SIGINT, &act, nullptr);
}

void WorkerProcess::write_to_log(log_messages_t log_type) {
    switch (log_type) {
        case INFO_HARD_STOP_DONE:
            BOOST_LOG_TRIVIAL(info) << "HARD STOP for worker DONE [WORKER PID " << getpid() << "]";
            break;
        case INFO_SOFT_STOP_START:
            BOOST_LOG_TRIVIAL(info) << "SOFT STOP for worker START [WORKER PID " << getpid() << "]";
            break;
        case INFO_SOFT_STOP_DONE:
            BOOST_LOG_TRIVIAL(info) << "SOFT STOP for worker DONE [WORKER PID " << getpid() << "]";
            break;
    }
}
