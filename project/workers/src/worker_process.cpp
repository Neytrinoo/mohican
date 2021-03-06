#include <sys/socket.h>
#include <sys/epoll.h>
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

bool is_hard_stop = false;
bool is_soft_stop = false;
bool is_soft_reload = false;

WorkerProcess::WorkerProcess(int listen_sock, class ServerSettings *server_settings,
                             std::vector<MohicanLog *> &vector_logs) :
        listen_sock(listen_sock), server_settings(server_settings), vector_logs(vector_logs) {
    signal(SIGPIPE, SIG_IGN);
    this->setup_sighandlers();
}

void WorkerProcess::run() {
    static struct epoll_event ev, events[EPOLL_SIZE];
    ev.events = EPOLLIN;

    int epoll_fd = epoll_create(EPOLL_SIZE);
    ev.data.fd = this->listen_sock;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, this->listen_sock, &ev);

    int client, epoll_events_count;

    while (!is_hard_stop && !is_soft_stop) {
        epoll_events_count = epoll_wait(epoll_fd, events, EPOLL_SIZE, EPOLL_RUN_TIMEOUT);
        for (int i = 0; i < epoll_events_count; ++i) {
            if (is_soft_stop || is_hard_stop) {
                break;
            }

            if (events[i].data.fd == this->listen_sock) {
                ClientConnection *client_connection = new(std::nothrow) ClientConnection(this->server_settings,
                                                                                         vector_logs);
                if (!client_connection) {
                    continue;
                }

                client = accept(this->listen_sock, NULL, NULL);
                if (client == -1) {
                    continue;
                }
                fcntl(client, F_SETFL, fcntl(client, F_GETFL, 0) | O_NONBLOCK);
                client_connection->set_socket(client);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.ptr = (ClientConnection *) client_connection;

                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client, &ev);
            } else {  // if the event happened on a client socket
                ClientConnection *client_connection = (ClientConnection *) events[i].data.ptr;
                connection_status_t connection_status = client_connection->connection_processing();
                if (connection_status == CONNECTION_FINISHED || connection_status == CONNECTION_TIMEOUT_ERROR ||
                    connection_status == ERROR_WHILE_CONNECTION_PROCESSING) {
                    close(client_connection->get_client_sock());
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_connection->get_client_sock(), &events[i]);
                    delete client_connection;
                } else if (connection_status == CHECKOUT_PROXY_FOR_READ || connection_status == CHECKOUT_PROXY_FOR_WRITE) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_connection->get_client_sock(), &events[i]);

                    ev.data.ptr = (ClientConnection *) client_connection;
                    if (connection_status == CHECKOUT_PROXY_FOR_READ) {
                        ev.events = EPOLLIN;
                    } else {
                        ev.events = EPOLLOUT;
                    }
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_connection->get_upstream_sock(), &ev);
                    this->write_to_logs("checkout to proxy", INFO);
                } else if (connection_status == CHECKOUT_CLIENT_FOR_READ || connection_status == CHECKOUT_CLIENT_FOR_WRITE) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_connection->get_upstream_sock(), &events[i]);

                    ev.data.ptr = (ClientConnection *) client_connection;
                    if (connection_status == CHECKOUT_CLIENT_FOR_READ) {
                        ev.events = EPOLLIN;
                    } else {
                        ev.events = EPOLLOUT;
                    }
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_connection->get_client_sock(), &ev);
                    this->write_to_logs("checkout to client", INFO);
                }
            }
        }
    }

    if (is_soft_stop || is_soft_reload) {
        this->message_to_log(INFO_SOFT_STOP_START);

        ev.data.fd = this->listen_sock;
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, this->listen_sock, &ev);

        epoll_events_count = epoll_wait(epoll_fd, events, EPOLL_SIZE, 1);
        for (int i = 0; i < epoll_events_count; ++i) {
            ClientConnection *client_connection = (ClientConnection *) events[i].data.ptr;
            connection_status_t connection_status = client_connection->connection_processing();
            if (connection_status == CONNECTION_FINISHED || connection_status == CONNECTION_TIMEOUT_ERROR ||
                connection_status == ERROR_WHILE_CONNECTION_PROCESSING) {
                close(client_connection->get_client_sock());
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_connection->get_client_sock(), &events[i]);

                delete client_connection;
            }
        }

        this->message_to_log(INFO_SOFT_STOP_DONE);
    } else {
        this->message_to_log(INFO_HARD_STOP_DONE);
    }
    exit(0);
}

void WorkerProcess::sighup_handler(int sig) {
    is_soft_stop = true;
}

void WorkerProcess::sigint_handler(int sig) {
    is_hard_stop = true;
}

void WorkerProcess::sigpoll_handler(int sig) {
    is_soft_reload = true;
}

void WorkerProcess::setup_sighandlers() {
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    act.sa_handler = sighup_handler;
    sigaction(SIGHUP, &act, nullptr);

    act.sa_handler = sigint_handler;
    sigaction(SIGINT, &act, nullptr);

    act.sa_handler = sigpoll_handler;
    sigaction(SIGPOLL, &act, nullptr);
}

void WorkerProcess::message_to_log(log_messages_t log_type) {
    switch (log_type) {
        case INFO_HARD_STOP_DONE:
            this->write_to_logs("HARD STOP for worker DONE [WORKER PID " + std::to_string(getpid()) + "]", INFO);
            break;
        case INFO_SOFT_STOP_START:
            this->write_to_logs("SOFT STOP for worker START [WORKER PID " + std::to_string(getpid()) + "]", INFO);
            break;
        case INFO_SOFT_STOP_DONE:
            this->write_to_logs("SOFT STOP for worker DONE [WORKER PID " + std::to_string(getpid()) + "]", INFO);
            break;
    }
}

bool WorkerProcess::is_banned_upstream(const std::string &ip, int result_code) {
    auto iter = std::find(banned_upstreams.begin(), banned_upstreams.end(), ip);

    if (result_code % 100 == 4 || result_code % 100 == 5) {
        if (iter == banned_upstreams.end()) {
            banned_upstreams.push_back(ip);
        }
        return true;
    }

    if (iter != banned_upstreams.end()) {
        banned_upstreams.erase(iter);
    }

    return false;
}

void WorkerProcess::write_to_logs(std::string message, bl::trivial::severity_level lvl) {
    for (auto i : vector_logs) {
        i->log(message, lvl);
    }
}
