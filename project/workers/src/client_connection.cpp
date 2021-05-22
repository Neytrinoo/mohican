#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <exception>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/sources/global_logger_storage.hpp>


#include "client_connection.h"
#include "http_request.h"
#include "http_response.h"
#include "http_handle.h"

#define MAX_METHOD_LENGTH 4
#define CLIENT_SEC_TIMEOUT 5 // maximum request idle time
#define PAGE_404 "public/404.html" // потом изменить
#define FOR_404_RESPONSE "/sadfsadf/sadfsaf/asdfsaddf"

ClientConnection::ClientConnection(int sock, class ServerSettings *server_settings, std::vector<MohicanLog>& vector_logs) :
        sock(sock), server_settings(server_settings), vector_logs(vector_logs) {}

connection_status_t ClientConnection::connection_processing() {
    if (this->stage == ERROR_STAGE) {
        return ERROR_WHILE_CONNECTION_PROCESSING;
    }

    if (this->stage == GET_REQUEST) {
        if (this->get_request()) {
            this->stage = FORM_HTTP_HEADER_RESPONSE;

        } else if (clock() / CLOCKS_PER_SEC - this->timeout > CLIENT_SEC_TIMEOUT) {
            this->message_to_log(ERROR_TIMEOUT);
            return CONNECTION_TIMEOUT_ERROR;
        }
    }

    if (this->stage == FORM_HTTP_HEADER_RESPONSE) {
        if (this->form_http_header_response()) {
            this->stage = SEND_HTTP_HEADER_RESPONSE;
        }
    }

    if (this->stage == SEND_HTTP_HEADER_RESPONSE) {
        if (this->send_http_header_response()) {
            this->stage = SEND_FILE;
        } else if (clock() / CLOCKS_PER_SEC - this->timeout > CLIENT_SEC_TIMEOUT) {
            this->message_to_log(ERROR_TIMEOUT);
            return CONNECTION_TIMEOUT_ERROR;
        }
    }

    if (this->stage == SEND_FILE) {
        if (this->send_file()) {
            this->message_to_log(INFO_CONNECTION_FINISHED);
            return CONNECTION_FINISHED;
        } else if (clock() / CLOCKS_PER_SEC - this->timeout > CLIENT_SEC_TIMEOUT) {
            this->message_to_log(ERROR_TIMEOUT);
            return CONNECTION_TIMEOUT_ERROR;
        }
    }

    return CONNECTION_PROCESSING;
}

bool ClientConnection::get_request() {
    char c;
    bool is_read_data = false;
    int result_read;
    while ((result_read = read(this->sock, &c, sizeof(c))) == sizeof(c)) {
        this->request.push_back(c);
        if (this->request.size() >= MAX_METHOD_LENGTH) {
            this->set_method();
        }
        is_read_data = true;
    }

    if (this->is_end_request()) {
        return true;
    }

    if (result_read == -1) {
        this->stage = ERROR_STAGE;
        return false;
    }

    if (is_read_data) {
        this->timeout = clock() / CLOCKS_PER_SEC;
    }

    return false;
}


bool ClientConnection::form_http_header_response() {
    HttpRequest http_request;
    try {
        http_request = HttpRequest(this->request);
    } catch (std::exception &e) {
        this->message_to_log(ERROR_BAD_REQUEST);
        // TODO: добавить обработку BAD REQUEST запроса (какой-то дефолтный ответ и страничка)
        return true;
    }


    std::string url = http_request.get_url();
    this->message_to_log(INFO_NEW_CONNECTION, url, http_request.get_method());
    HttpResponse http_response;
    try {
        std::string root = this->server_settings->get_root(url);
        http_response = http_handler(http_request, root);
        this->file_fd = open((root + url).c_str(), O_RDONLY);
        if (this->file_fd == -1) {
            this->file_fd = open(PAGE_404, O_RDONLY);
            this->message_to_log(ERROR_404_NOT_FOUND);
        }
    } catch (std::exception &e) {
        http_response = http_handler(http_request);
        this->file_fd = open(PAGE_404, O_RDONLY);
        this->message_to_log(ERROR_404_NOT_FOUND);
    }
    this->response = http_response.get_string();
    this->request.clear();


    return true;
}


bool ClientConnection::send_http_header_response() {
    bool is_write_data = false;
    int write_result;
    while ((write_result = write(this->sock, this->response.c_str() + response_pos, 1)) == 1) {
        response_pos++;
        if (response_pos == this->response.size() - 1) {
            this->response.clear();
            write_result = write(this->sock, "\r\n", 2);
            if (write_result == -1) {
                this->stage = ERROR_STAGE;
                this->message_to_log(ERROR_SEND_RESPONSE);
                return false;
            }
            return true;
        }
        is_write_data = true;
    }


    if (write_result == -1) {
        this->stage = ERROR_STAGE;
        this->message_to_log(ERROR_SEND_RESPONSE);
        return false;
    }

    if (is_write_data) {
        this->timeout = clock() / CLOCKS_PER_SEC;
    }

    return false;
}


bool ClientConnection::send_file() {
    bool is_write_data = false;
    char c;
    int read_code;
    int write_result;

    read_code = read(this->file_fd, &c, sizeof(c));
    while ((write_result = write(this->sock, &c, sizeof(c)) == sizeof(c)) && read_code > 0) {
        read_code = read(this->file_fd, &c, sizeof(c));
        is_write_data = true;
    }

    if (write_result == -1) {
        this->stage = ERROR_STAGE;
        this->message_to_log(ERROR_SEND_FILE);
        return false;
    }

    if (read_code == 0) {
        return true;
    }

    if (is_write_data) {
        this->timeout = clock() / CLOCKS_PER_SEC;
    }

    return false;
}


void ClientConnection::set_method() {
    if (this->request.substr(0, 3) == "GET") {
        this->method = GET;
    } else if (this->request.substr(0, 4) == "POST") {
        this->method = POST;
    }
}

bool ClientConnection::is_end_request() {
    size_t pos = this->request.find("\r\n\r\n");
    return pos != std::string::npos;
}

void ClientConnection::message_to_log(log_messages_t log_type, std::string url, std::string method) {
    switch (log_type) {
        case INFO_NEW_CONNECTION:
            this->write_to_logs("New connection [METHOD " + method + "] [URL "
                                    + url
                                    + "] [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET " + std::to_string(this->sock)
                                    + "]", INFO);
            break;
        case INFO_CONNECTION_FINISHED:
            this->write_to_logs("Connection finished successfully [WORKER PID " + std::to_string(getpid()) + "]" + " [CLIENT SOCKET "
                                    + std::to_string(this->sock) + "]", INFO);
            break;
        case ERROR_404_NOT_FOUND:
            this->write_to_logs("404 NOT FOUND [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET "
                                     + std::to_string(this->sock) + "]", ERROR);
            break;
        case ERROR_TIMEOUT:
            this->write_to_logs("TIMEOUT ERROR [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET "
                                + std::to_string(this->sock) + "]", ERROR);
            break;
        case ERROR_READING_REQUEST:
            this->write_to_logs("Reading request error [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET "
                                + std::to_string(this->sock) + "]", ERROR);
            break;
        case ERROR_SEND_RESPONSE:
            this->write_to_logs("Send response error [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET "
                                + std::to_string(this->sock) + "]", ERROR);
            break;
        case ERROR_SEND_FILE:
            this->write_to_logs("Send file error [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET "
                                + std::to_string(this->sock) + "]", ERROR);
            break;
        case ERROR_BAD_REQUEST:
            this->write_to_logs("Bad request error [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET "
                                + std::to_string(this->sock) + "]", ERROR);
            break;
    }
}

clock_t ClientConnection::get_timeout() {
    return this->timeout;
}

void ClientConnection::write_to_logs(std::string message, bl::trivial::severity_level lvl) {
    for (auto i : vector_logs) {
        i.log(message, lvl);
    }
}
