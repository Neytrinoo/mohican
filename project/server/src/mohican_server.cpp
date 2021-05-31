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
        old_master_process = getpid();
        return 0;
    }

    if (server_action == RELOAD_SERVER) {
        pid_t pid_child_of_old_master = fork();

        if (pid_child_of_old_master == -1) {
            return -1;
        }

        // check it isn't old master
        if (getpid() == old_master_process) {
            return 0;
        }

        pid_t pid_to_finish = getpid();

        /*close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);*/
        pid_t new_master_pid = fork();

        if (new_master_pid == -1) {
            return -1;
        }
        if (getpid() == pid_to_finish) {
            exit(0);
        }
        
        new_master_process = getpid();
        setsid();

        return (int)new_master_process;
    }
    return 0;
}

int MohicanServer::fill_pid_file(status_server_action server_action, action_level_t lvl) {
    std::ofstream stream_to_pid_file;
    stream_to_pid_file.open("pid_file.txt", std::ios::out);

    if (!stream_to_pid_file.is_open()) {
        return -1;
    }

    stream_to_pid_file << getpid() << std::endl;

    if (server_action == START_SERVER || (server_action == RELOAD_SERVER && lvl == HARD_LEVEL)) {
        for (auto i : workers_pid) {
            stream_to_pid_file << i << std::endl;
        }
    }

    if (server_action == RELOAD_SERVER && lvl == SOFT_LEVEL) {
        for (auto i : new_workers_pid) {
            stream_to_pid_file << i << std::endl;
        }
    }

    stream_to_pid_file.close();

    return 0;
}

int MohicanServer::delete_pid_file() {
    return remove("pid_file.txt");
}

int MohicanServer::add_work_processes(status_server_action server_action, action_level_t lvl) {
    int count_work_processes, param;
    if (server_action == START_SERVER || (server_action == RELOAD_SERVER && lvl == HARD_LEVEL)) {
        count_work_processes = mohican_settings.get_count_workflows();

        if (count_work_processes <= 0) {
            write_to_logs("COUNT WORKFLOWS NEED BE MORE 0", ERROR);
            return -1;
        }

        param = count_work_processes;
    }
    
    if (server_action == RELOAD_SERVER && lvl == SOFT_LEVEL) {
        count_work_processes = new_mohican_settings.get_count_workflows();

        if (count_work_processes <= 0) {
            write_to_logs("COUNT WORKFLOWS NEED BE MORE 0", ERROR);
            return -1;
        }

        param = count_work_processes;
    }

    // TODO: check on clear vector_pid
    for (int i = 0; i < param; ++i) {
       pid_t pid = fork();
        if (pid == -1) {
           write_to_logs("ERROR FORK", ERROR);
           return -1;
        }
        if (pid != 0) {
            if (server_action == RELOAD_SERVER && lvl == SOFT_LEVEL) {
                new_workers_pid.push_back(pid);
            } else {
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
    if (daemonize(START_SERVER) != 0) {
        write_to_logs("ERROR IN SERVER DAEMONIZE", ERROR);
        return -1;
    }

    if (!this->bind_listen_sock()) {
        write_to_logs("ERROR IN BIND SOCKET", ERROR);
        return -1;
    }

    if (add_work_processes(START_SERVER, NULL_LEVEL)!= 0) {
        write_to_logs("ERROR IN ADDING WORK PROCESSES", ERROR);
        return -1;
    }

    if (fill_pid_file(START_SERVER, NULL_LEVEL) == -1) {
        write_to_logs("ERROR IN FILL PID FILE", ERROR);
        return -1;
    }

    write_to_logs("Worker processes (" + std::to_string(this->workers_pid.size()) + ") successfully started", INFO);

    process_setup_signals();  // установка нужных обработчиков сигналов

    while (true) {
        if (process_soft_stop == 1) {
            server_stop(SOFT_LEVEL);
            return 0;
        }

        if (process_hard_stop == 1) {
            server_stop(HARD_LEVEL);
            return 0;
        }

        if (process_soft_reload == 1) {
            if (server_reload(SOFT_LEVEL) == -1) {
                write_to_logs("ERROR SOFT RELOAD", ERROR);
                server_stop(HARD_LEVEL);
                return -1;
            }
        }

        if (process_hard_reload == 1) {
            if (server_reload(HARD_LEVEL) == -1) {
                write_to_logs("ERROR HARD RELOAD", ERROR);
                server_stop(HARD_LEVEL);
                return -1;
            }
            process_hard_reload = 0;
        }
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

    act.sa_handler = sighup_handler;
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

        int status;
        for (auto &i : this->workers_pid) {
            kill(i, SIGINT);
        }
        for (auto &i : this->workers_pid) {
            waitpid(i, &status, 0);
        }

        write_to_logs("SERVER STOPPED!", INFO);
        delete_pid_file();

        exit(0);
    }

    if (level == SOFT_LEVEL) {
        write_to_logs("SOFT SERVER STOP...", WARNING);

        int status;
        for (auto &i : this->workers_pid) {
            kill(i, SIGHUP);
        }
        for (auto &i : this->workers_pid) {
            waitpid(i, &status, 0);
        }

        if (process_soft_reload == 2) {
            write_to_logs("OLD MASTER SERVER STOPPED!", INFO);
        } else {
            close(this->listen_sock);
            delete_pid_file();
            write_to_logs("SERVER STOPPED!", INFO);
        }

        exit(0);
    }

    write_to_logs("ERROR! SERVER NOT STOPPED!", ERROR);

    return -1;
}

int MohicanServer::server_reload(action_level_t level) {
    if (level == HARD_LEVEL) {
        write_to_logs("HARD SERVER RELOADING...", WARNING);

        mohican_settings = MainServerSettings(CONFIG_FILE_PATH);

        if (apply_config(RELOAD_SERVER, HARD_LEVEL) == -1) {
            write_to_logs("ERROR APPLY CONFIG", ERROR);
            return -1;
        }

        write_to_logs("SERVER RELOADED!", INFO);
        return 0;
    }

    if (level == SOFT_LEVEL) {
        write_to_logs("SOFT SERVER RELOADING...", WARNING);

        new_mohican_settings = MainServerSettings(CONFIG_FILE_PATH);

        pid_t status_daemonize = daemonize(RELOAD_SERVER);

        if (status_daemonize == -1) {
            write_to_logs("ERROR DAEMONIZE", ERROR);
            return -1;
        }
        if (getpid() == old_master_process) {
            process_soft_reload = 2;
            return 0;
        }

        // need to us new master process
        // status daemonize return pid new master process

        
        write_to_logs("SOFT SERVER RELOADING...New master process successfully started PID " +
            std::to_string(getpid()) + " and new master process " + std::to_string(new_master_process), WARNING);

        if (apply_config(RELOAD_SERVER, SOFT_LEVEL) == -1) {
            write_to_logs("ERROR APPLY CONFIG", ERROR);
            return -1;
        }

        int status;
        kill(old_master_process, SIGHUP);
        waitpid(old_master_process, &status, 0);

        write_to_logs("OLD MASTER FINISHED ALL CONNECTIONS WITH STATUS: " +
            std::to_string(WEXITSTATUS(status)) + " PID " + std::to_string(getpid()), INFO);

        fill_pid_file(RELOAD_SERVER, SOFT_LEVEL);

        mohican_settings = new_mohican_settings;
        count_workflows = mohican_settings.get_count_workflows();
        server = mohican_settings.get_server();

        workers_pid = new_workers_pid;

        old_master_process = new_master_process;
        new_master_process = 0;

        write_to_logs("SERVER RELOADED!", INFO);
        process_soft_reload = 0;

        return 0;
    }

    write_to_logs("ERROR IN SERVER RELOADING", ERROR);
    return -1;
}

int MohicanServer::apply_config(status_server_action server_action, action_level_t level) {
    if (level == SOFT_LEVEL) {
        write_to_logs("SOFT STOP RELOADING...NEW CONFIG APPLYING", WARNING);

        count_workflows = new_mohican_settings.get_count_workflows();
        if (count_workflows <= 0) {
            write_to_logs("COUNT WORK PROCESSES MUST BE MORE 0", ERROR);
            return -1;
        }

        server = new_mohican_settings.get_server();

        if (add_work_processes(RELOAD_SERVER, SOFT_LEVEL) == -1) {
            write_to_logs("ERROR ADD WORKERS", ERROR);
            return -1;
        }
        if (fill_pid_file(RELOAD_SERVER, SOFT_LEVEL) == -1) {
            write_to_logs("ERROR FILL PID FILE", ERROR);
            return -1;
        }

        write_to_logs("COUNT WORK PROCESSES WAS SUCCESSFULLY CHECKED", WARNING);
        write_to_logs("Worker processes (" + std::to_string(this->new_workers_pid.size()) + ") successfully started", INFO);
    }
    
    if (level == HARD_LEVEL) {
        write_to_logs("HARD STOP RELOADING...NEW CONFIG APPLYING", WARNING);

        count_workflows = mohican_settings.get_count_workflows();
        if (count_workflows <= 0) {
            write_to_logs("COUNT WORK PROCESSES MUST BE MORE 0", ERROR);
            return -1;
        }

        server = mohican_settings.get_server();

        int status;
        for (auto &i : this->workers_pid) {
            kill(i, SIGINT);
        }
        for (auto &i : this->workers_pid) {
            waitpid(i, &status, 0);
        }

        workers_pid.clear();

        if (add_work_processes(RELOAD_SERVER, HARD_LEVEL) == -1) {
            write_to_logs("ERROR ADD WORKERS", ERROR);
            return -1;
        }
        if (fill_pid_file(RELOAD_SERVER, HARD_LEVEL) == -1) {
            write_to_logs("ERROR FILL PID FILE", ERROR);
            return -1;
        }

        write_to_logs("COUNT WORK PROCESSES WAS SUCCESSFULLY CHECKED", WARNING);
        write_to_logs("Worker processes (" + std::to_string(this->workers_pid.size()) + ") successfully started", INFO);
    }

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
