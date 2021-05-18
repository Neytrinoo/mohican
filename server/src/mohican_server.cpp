#include <cstdlib>
#include <unistd.h>
#include <ctime>
#include <iostream>
#include <csignal>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "mohican_server.h"

#define BACKLOG 128
#define CONFIG_FILE_PATH "settings/mohican.conf"


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

    stream_to_pid_file << getpid() << std::endl;
    for (auto i : workers_pid) {
        stream_to_pid_file << i << std::endl;
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

int MohicanServer::log_open(std::string path_to_log, std::string level_log, bool key) {
    if (level_log != "info" && level_log != "debug" && level_log != "error") {
        return -1;
    }

    boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");

    logger_t lg;

    boost::log::add_file_log(
            boost::log::keywords::auto_flush = key,
            boost::log::keywords::file_name = path_to_log,
            boost::log::keywords::format = "[%TimeStamp%] [%Severity%] %Message%" /*[%Uptime%]*/
    );

    if (level_log != "debug") {
        if (level_log == "info") {
            boost::log::core::get()->set_filter(boost::log::trivial::severity == boost::log::trivial::info);
        } else {
            boost::log::core::get()->set_filter(boost::log::trivial::severity == boost::log::trivial::error);
        }
    }

    boost::log::add_common_attributes();

    logging::core::get()->add_global_attribute("Uptime", attrs::timer());

    BOOST_LOG_TRIVIAL(info) << "SERVER STARTED!";

    return 0;
}

int MohicanServer::server_start() {
    if (daemonize() != 0) {
        return -1;
    }


    if (!this->bind_listen_sock()) {
        return -1;
    }

    // TODO: добавить в конфиг уровни логгирования
    if (log_open(mohican_settings.get_log_filename(), mohican_settings.get_log_level(), true) != 0) {
        return -1;
    }

    if (add_work_processes() != 0) {
        return -1;
    }

    if (fill_pid_file() == -1) {
        return -1;
    }

    BOOST_LOG_TRIVIAL(info) << "Worker processes (" << this->workers_pid.size() << ") successfully started";

    process_setup_signals();  // установка нужных обработчиков сигналов

    int number_process = 1;
    std::string status = "not_exited";

    while (process_soft_stop != 1 && process_hard_stop != 1 && process_reload != 1);
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
    if (level == HARD_LEVEL) {
        BOOST_LOG_TRIVIAL(warning) << "HARD SERVER STOP...";
        close(this->listen_sock);
        for (auto &i : this->workers_pid) {
            kill(i, SIGINT);
        }
        BOOST_LOG_TRIVIAL(info) << "SERVER STOPPED!";
        exit(0);
    }

    if (level == SOFT_LEVEL) {
        BOOST_LOG_TRIVIAL(warning) << "SOFT SERVER STOP...";
        close(this->listen_sock);
        for (auto &i : this->workers_pid) {
            kill(i, SIGHUP);
        }
        exit(0);
    }

    BOOST_LOG_TRIVIAL(error) << "ERROR! SERVER NOT STOPPED!";

    return -1;
}


int MohicanServer::server_reload() {
    BOOST_LOG_TRIVIAL(warning) << "SERVER RELOADING...";

    MainServerSettings mohican_new_settings;
    mohican_settings = mohican_new_settings;
    apply_config();

    BOOST_LOG_TRIVIAL(info) << "SERVER RELOADED!";

    return 0;
}

int MohicanServer::apply_config() {
    log_open(mohican_settings.get_log_filename(), mohican_settings.get_log_level(), false);

    // настройка количества рабочих процессов
    count_workflows = mohican_settings.get_count_workflows();

    if (count_workflows == 0) {
        BOOST_LOG_TRIVIAL(error) << "COUNT WORK PROCESSES MUST BE MORE 0";
        return -1;
    }

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
    BOOST_LOG_TRIVIAL(warning)
        << "Upstream [SERVERNAME : Local_host] [IP : 192.89.89.89] not respond to request from worker " << getpid();
    BOOST_LOG_TRIVIAL(info) << "Successfully connection to upstream [SERVERNAME : Local_host] [IP : 192.89.89.89]";
    BOOST_LOG_TRIVIAL(error) << "Upstream [SERVERNAME : Local_host] [IP : 192.89.89.89] was added to ban-list";
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
