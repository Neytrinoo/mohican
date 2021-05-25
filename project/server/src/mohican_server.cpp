#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <csignal>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "mohican_server.h"

#define BACKLOG 128
#define CONFIG_FILE_PATH "settings/mohican.conf"


int process_soft_stop = 0;
int process_hard_stop = 0;
int process_soft_reload = 0;
int process_hard_reload = 0;

MohicanServer::MohicanServer() {
    this->mohican_settings = MainServerSettings(CONFIG_FILE_PATH);
    this->count_workflows = this->mohican_settings.get_count_workflows();
    this->server = this->mohican_settings.get_server();
    vector_logs.push_back(&error_log);
    vector_logs.push_back(&access_log);
    write_to_logs("SERVER STARTING...", INFO);
}

bl::trivial::severity_level MohicanServer::cast_types_logs_level(std::string lvl) {
    if (lvl == "info") {
        return INFO;
    }
    if (lvl == "debug") {
        return DEBUG;
    }
    if (lvl == "trace") {
        return TRACE;
    }
    return ERROR;
}

void MohicanServer::write_to_logs(std::string message, bl::trivial::severity_level lvl) {
    for (auto i : vector_logs) {
        i->log(message, lvl);
    }
}

/*void MohicanServer::init_logs(bool flush_flag) {
    MohicanLog access_log("access", flush_flag, cast_types_logs_level(access_log_level));
    MohicanLog error_log("error", flush_flag, cast_types_logs_level(error_log_level));
    vector_logs.push_back(&access_log);
    vector_logs.push_back(&error_log);
    write_to_logs("SERVER STARTING...", INFO);
}*/

int MohicanServer::daemonize(status_server_action server_action) {
    if (server_action == START_SERVER) {
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

    if (server_action == RELOAD_SERVER) {
        pid_t pid_child_of_old_master = fork();

        if (pid_child_of_old_master == -1) {
            return -1;
        }
        // check it isn't old master
        if (getppid() != 1) {
            pid_t pid_to_finish = getppid();
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
            pid_t new_master_pid = fork();

            if (new_master_pid == -1) {
                return -1;
            }
            if (getppid() == pid_to_finish) {
                exit(0);
            }
        }
        return int(getpid());
    }
    return 0;
}

int MohicanServer::fill_pid_file(status_server_action server_action) {
    std::ofstream stream_to_pid_file;
    stream_to_pid_file.open("pid_file.txt", std::ios::out);

    if (!stream_to_pid_file.is_open()) {
        return -1;
    }

    stream_to_pid_file << getpid() << std::endl;

    if (server_action == START_SERVER) {
        for (auto i : workers_pid) {
            stream_to_pid_file << i << std::endl;
        }
    }

    if (server_action == RELOAD_SERVER) {
        for (auto i : new_workers_pid) {
            stream_to_pid_file << i << std::endl;
        }
    }

    stream_to_pid_file.close();

    return 0;
}

int MohicanServer::add_work_processes(status_server_action server_action) {
    int  count_work_processes;
    if (server_action == START_SERVER) {
        count_work_processes = mohican_settings.get_count_workflows();
    }
    if (server_action == RELOAD_SERVER) {
        count_work_processes = new_mohican_settings.get_count_workflows();
    }

    //TODO: check on clear vector_pid

    for (int i = 0; i < count_work_processes; ++i) {
        pid_t pid = fork();
        if (pid == -1) {
            return -1;
        }
        if (pid != 0) {
            if (server_action == RELOAD_SERVER) {
                new_workers_pid.push_back(pid);
            }
            if (server_action == START_SERVER) {
                workers_pid.push_back(pid);
            }
        } else {
            WorkerProcess worker(this->listen_sock, &server, vector_logs);
            worker.run();
            break;
        }
    }

    return 0;
}

int MohicanServer::server_start() {
    //init_logs(true);

    if (daemonize(START_SERVER) != 0) {
        write_to_logs("ERROR IN SERVER DAEMONIZE", ERROR);
        return -1;
    }

    if (!this->bind_listen_sock()) {
        write_to_logs("ERROR IN BIND SOCKET", ERROR);
        return -1;
    }

    if (add_work_processes(START_SERVER)!= 0) {
        write_to_logs("ERROR IN ADDING WORK PROCESSES", ERROR);
        return -1;
    }

    if (fill_pid_file(START_SERVER) == -1) {
        write_to_logs("ERROR IN FILL PID FILE", ERROR);
        return -1;
    }

    write_to_logs("Worker processes (" + std::to_string(this->workers_pid.size()) + ") successfully started", INFO);

    process_setup_signals();  // установка нужных обработчиков сигналов

    int number_process = 1; // TODO: who add and why??
    std::string status = "not_exited";

    while (process_soft_stop != 1 && process_hard_stop != 1 && process_soft_reload != 1 && process_hard_reload != 1);

    if (process_soft_stop == 1) {
        server_stop(SOFT_LEVEL);
    }

    if (process_hard_stop == 1) {
        server_stop(HARD_LEVEL);
    }

    if (process_soft_reload == 1) {
        server_reload(SOFT_LEVEL);
    }

    if (process_hard_reload == 1) {
        server_reload(HARD_LEVEL);
    }

    return 0;
}

void MohicanServer::sighup_handler(int sig) {
    process_soft_stop = 1;
}

void MohicanServer::sigint_handler(int sig) {
    process_hard_stop = 1;
}

void MohicanServer::sigpipe_handler(int sig) {
    process_soft_reload = 1;
}

void MohicanServer::sigalrm_handler(int sig) {
    process_hard_reload = 1;
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
    act.sa_handler = sigalrm_handler;
    sigaction(SIGALRM, &act, nullptr);
    return 0;
}

int MohicanServer::server_stop(action_level_t level) {
    if (level == HARD_LEVEL) {
        write_to_logs("HARD SERVER STOP...", WARNING);
        close(this->listen_sock);
        for (auto &i : this->workers_pid) {
            kill(i, SIGINT);
        }
        write_to_logs("SERVER STOPPED", INFO);
        exit(0);
    }

    if (level == SOFT_LEVEL) {
        write_to_logs("SOFT SERVER STOP...", WARNING);
        close(this->listen_sock);
        for (auto &i : this->workers_pid) {
            kill(i, SIGHUP);
        }
        write_to_logs("SERVER STOPPED", INFO);
        exit(0);
    }

    write_to_logs("ERROR! SERVER NOT STOPPED!", ERROR);

    return -1;
}


int MohicanServer::server_reload(action_level_t level) {
    if (level == HARD_LEVEL) {
        write_to_logs("HARD SERVER RELOADING...", WARNING);

        MainServerSettings mohican_new_settings;
        mohican_settings = mohican_new_settings;

        apply_config(RELOAD_SERVER, HARD_LEVEL);

        write_to_logs("SERVER RELOADED!", INFO);
        return 0;
    }

    if (level == SOFT_LEVEL) {
        write_to_logs("SOFT SERVER RELOADING...", WARNING);

        new_mohican_settings = MainServerSettings();

        pid_t status_daemonize = daemonize(RELOAD_SERVER);

        if (status_daemonize == -1) {
            return -1;
        }
        write_to_logs("SOFT SERVER RELOADING...New master process successfully started", WARNING);
        // need to us new master process
        // status daemonize return pid new master process
        if (getpid() == status_daemonize) {

            apply_config(RELOAD_SERVER, SOFT_LEVEL);

            int status;
            for (auto &i : this->workers_pid) {
                kill(i, SIGPOLL);
            }
            for (auto &i : this->workers_pid) {
                waitpid(i, &status, 0);
            }

            mohican_settings = new_mohican_settings;
            workers_pid = new_workers_pid;
            write_to_logs("SERVER RELOADED!", INFO);
            return 0;
        }
    }

    write_to_logs("ERROR IN SERVER RELOADING", ERROR);
    return -1;
}

int MohicanServer::apply_config(status_server_action server_action, action_level_t level) {
    if (level == SOFT_LEVEL) {
        write_to_logs("SOFT STOP RELOADING...NEW CONFIG APPLYING", WARNING);
        count_workflows = new_mohican_settings.get_count_workflows();
        add_work_processes(RELOAD_SERVER);
        fill_pid_file(RELOAD_SERVER);
        write_to_logs("Worker processes (" + std::to_string(this->new_workers_pid.size()) + ") successfully started", INFO);
        return 0;
    }

    // настройка количества рабочих процессов
    if (level == HARD_LEVEL) {
        count_workflows = mohican_settings.get_count_workflows();
        write_to_logs("START SERVER...CONFIG APPLYING", WARNING);
    }

    if (count_workflows == 0) {
        write_to_logs("COUNT WORK PROCESSES MUST BE MORE 0", ERROR);
        return -1;
    }

    if (!(access_lvl_before_reload == access_log_level && error_lvl_before_reload == error_log_level)) {
        vector_logs.clear();
        //init_logs(false);
        access_log = MohicanLog ("access", false, cast_types_logs_level(access_log_level));
        error_log = MohicanLog ("error", false, cast_types_logs_level(error_log_level));
        //TODO: connect with config
    }

    if (count_workflows > workers_pid.size() && workers_pid.size() != 1) {
        for (int i = 0; i < count_workflows - workers_pid.size(); ++i) {
            add_work_processes(server_action);
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

    fill_pid_file(RELOAD_SERVER);

    write_to_logs("COUNT WORK PROCESSES WAS SUCCESSFULLY CHECKED", WARNING);
    write_to_logs("Worker processes (" + std::to_string(this->workers_pid.size()) + ") successfully started", INFO);

    return 0;
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
