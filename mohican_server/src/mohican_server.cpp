#include <cstdlib>
#include <unistd.h>
#include <ctime>
#include <iostream>
#include <csignal>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "mohican_server.h"

#define BACKLOG 128
#define CONFIG_FILE_PATH "../settings/config.conf"


int process_soft_stop = 0;
int process_hard_stop = 0;
int process_reload = 0;

MohicanServer::MohicanServer() {
    this->mohican_settings = MainServerSettings(CONFIG_FILE_PATH);
    this->count_workflows = this->mohican_settings.get_count_workflows();
    this->server = this->mohican_settings.get_server();
}

int MohicanServer::daemonize() {
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    pid_t pid = fork();

    if (pid == -1) {
        return -1;
    }

    if (pid != 0) {  // Возвращается нуль процессу-потомку и пид чилда мастеру
        exit(0);
    }

    setsid();

    return 0;
}

int MohicanServer::fill_pid_file() {
    std::ofstream stream_to_pid_file;
    stream_to_pid_file.open("pid_file.txt", std::ios::out);

    if (!stream_to_pid_file.is_open()) {
        return -1;
    }

    if (getppid() == 1) {
        stream_to_pid_file << getpid() << std::endl;
        for (auto i : workers_pid) {
            stream_to_pid_file << i << std::endl;
        }
    }

    stream_to_pid_file.close();

    return 0;
}

int MohicanServer::add_work_processes() {
    for (int i = 0; i < mohican_settings.get_count_workflows(); ++i) {
        pid_t pid = fork();
        if (pid == -1) {
            return -1;
        }
        if (pid != 0) {
            workers_pid.push_back(pid);
        } else {
            WorkerProcess worker(this->listen_sock, &server);
            worker.run();
            break;
        }
    }

    return 0;
}

int MohicanServer::log_open() {

    stream_to_access_log.open(server.get_access_log_filename(), std::ios::app);  // std::ios::app - на добавление
    stream_to_error_log.open(server.get_error_log_filename(), std::ios::app);

    if (!stream_to_access_log.is_open()) {
        if (!stream_to_error_log.is_open()) {
            std::cout << "CHECK PATHS TO LOG " << server.get_access_log_filename()
                      << " " << server.get_error_log_filename();
            return -1;
        } else {
            log_message(server.get_servername(), ERROR_LEVEL, "CONFIGURATION IS FALSE, ERROR PATH TO ACCESS LOG "
                                                              + server.get_access_log_filename());
        }
    } else {
        log_message(server.get_servername(), ACCESS_LEVEL, "SERVER STARTED!");
    }

    return 0;
}

void MohicanServer::all_logs_close() {
    log_message(server.get_servername(), ACCESS_LEVEL, "SERVER CLOSED!");  // TODO: main_log

    if (stream_to_access_log.is_open()) {
        stream_to_access_log.close();
    }
    if (stream_to_error_log.is_open()) {
        stream_to_error_log.close();
    }
}

int MohicanServer::certain_log_close(std::ofstream stream_to_log) {
    if (stream_to_log.is_open()) {
        stream_to_log.close();
        return 0;
    }
    return -1;
}

int MohicanServer::log_message(std::string server_name, log_level_t level, std::string status) {
    time_t t = time(nullptr);
    struct tm tm;
    localtime_r(&t, &tm);

    if (level == ACCESS_LEVEL) {
        stream_to_access_log << server_name << " " << tm.tm_year << "-" << tm.tm_mon << "-" << tm.tm_yday
                             << "\t" << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << " " << status;
    } else {
        stream_to_error_log << server_name << " " << tm.tm_year << "-" << tm.tm_mon << "-" << tm.tm_mday
                            << "\t" << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << " " << status;
    }  // TODO:check lib

    return 0;
}

int MohicanServer::start_balancing(int *number_process) {
    if (*number_process == workers_pid.size() - 1) {
        *number_process = 1;
    }
    *number_process++;
    return *number_process;
}

int MohicanServer::server_start() {
    if (daemonize() != 0) {
        return -1;
    }


    if (fill_pid_file() == -1) {
        return -1;
    }

    if (!this->bind_listen_sock()) {
        return -1;
    }

    if (add_work_processes() != 0) {
        return -1;
    }

    if (log_open() != 0) {
        return -1;
    }

    process_setup_signals();  // установка нужных обработчиков сигналов

    int number_process = 1;
    std::string status = "not_exited";

    // TODO: проверка поднятия сокета, запись в логи

    while (process_soft_stop != 1 && process_hard_stop != 1 && process_reload != 1) {


        if (process_soft_stop == 1) {
            server_stop(SOFT_LEVEL);
        }

        if (process_hard_stop == 1) {
            server_stop(HARD_LEVEL);
        }

        if (process_reload == 1) {
            server_reload();
        }
    }
}

void MohicanServer::sighup_handler(int sig) {
    process_soft_stop = 1;
}

void MohicanServer::sigint_handler(int sig) {
    process_hard_stop = 1;
}

void MohicanServer::sigpipe_handler(int sig) {
    process_reload = 1;
}

int MohicanServer::process_setup_signals() {
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = sigpipe_handler;
    sigaction(SIGPIPE, &act, nullptr);
    act.sa_handler = MohicanServer::sighup_handler;
    sigaction(SIGHUP, &act, nullptr);
    act.sa_handler = sigint_handler;
    sigaction(SIGINT, &act, nullptr);
    return 0;
}

int MohicanServer::server_stop(stop_level_t level) {
    log_message(server.get_servername(), ACCESS_LEVEL, "SERVER STOPPED!");
    if (level == HARD_LEVEL) {
        all_logs_close();
        if (getppid() == 0) {
            exit(0);
        }
    }

    if (level == SOFT_LEVEL) {
        int block_socket = 0;
        // TODO:остановка в соответствии с ключами --soft
    }

    return 0;
}


int MohicanServer::server_reload() {
    all_logs_close();
    MainServerSettings mohican_new_settings;
    mohican_settings = mohican_new_settings;
    apply_config();

    return 0;
}

int MohicanServer::apply_config() {
    log_open();

    // настройка количества рабочих процессов
    count_workflows = mohican_settings.get_count_workflows();
    if (count_workflows > workers_pid.size() && workers_pid.size() != 1) {
        for (int i = 0; i < count_workflows - workers_pid.size(); ++i) {
            add_work_processes();
        }
    } else if (count_workflows < workers_pid.size()) {
        pid_t ppid = getppid();
        pid_t pid = getpid();
        for (int i = 0; i < workers_pid.size() - count_workflows; ++i) {
            if (ppid != 1) {
                close(STDIN_FILENO);
                close(STDOUT_FILENO);
                close(STDERR_FILENO);

                for (int j = 0; j < workers_pid.size(); ++j) {
                    if (workers_pid[j] == pid) {
                        workers_pid.erase(workers_pid.begin() + j);
                    }
                }

                exit(0);
            }
        }
    }

    // TODO:Проверка апстримов
}

bool MohicanServer::bind_listen_sock() {
    this->listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (this->listen_sock == -1)
        return false;

    int enable = 1;
    if (setsockopt(this->listen_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
        return false;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(this->server.get_port());
    if (inet_aton(server.get_servername().c_str(), &addr.sin_addr) == -1)
        return false;

    if (bind(this->listen_sock, (struct sockaddr *) &addr, sizeof(addr)) == -1)
        return false;

    if (listen(this->listen_sock, BACKLOG) == -1) {
        return false;
    }

    return true;
}
