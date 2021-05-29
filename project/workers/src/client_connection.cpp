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

#define CLIENT_SEC_TIMEOUT 5 // maximum request idle time
#define PAGE_404 "public/404.html"
#define LENGTH_LINE_FOR_RESERVE 256

ClientConnection::ClientConnection(int sock, class ServerSettings *server_settings,
                                   std::vector<MohicanLog *> &vector_logs) :
        sock(sock), server_settings(server_settings), vector_logs(vector_logs) {

}

connection_status_t ClientConnection::connection_processing() {
    if (this->stage == ERROR_STAGE) {
        return ERROR_WHILE_CONNECTION_PROCESSING;
    }

    if (this->stage == GET_REQUEST) {
        if (get_request()) {
            if (this->stage == GET_REQUEST) {
                this->make_response_header(true);
            } else {
                this->make_response_header(false);
            }
            this->stage = SEND_HTTP_HEADER_RESPONSE;
        } else if (clock() / CLOCKS_PER_SEC - this->timeout > CLIENT_SEC_TIMEOUT) {
            this->message_to_log(ERROR_TIMEOUT);
            return CONNECTION_TIMEOUT_ERROR;
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
    bool is_read_data = false;
    int result_read;
    this->line_.reserve(LENGTH_LINE_FOR_RESERVE);
    while ((result_read = read(this->sock, &last_char_, sizeof(last_char_))) == sizeof(last_char_)) {
        this->line_.push_back(last_char_);
        if (this->last_char_ == '\n') {
            this->request_.add_line(line_);
            this->line_.clear();
            this->line_.reserve(LENGTH_LINE_FOR_RESERVE);
            if (this->location_ == nullptr) {
                this->stage = this->process_location();
                if (this->stage == PASS_TO_PROXY) {
                    // TODO: реализовать проксирование
                } else if (this->stage == ROOT_FOUND) {
                    // если root найден, продолжаем обрабатывать запрос через get_request
                    this->stage = GET_REQUEST;
                } else {
                    return true;
                }
            }
        }
        is_read_data = true;
    }

    if (request_.requst_ended()) {
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

bool ClientConnection::make_response_header(bool root_found) {
    if (root_found) {
        this->response = http_handler(request_, location_->root).get_string();
        this->file_fd = open((location_->root + request_.get_url()).c_str(), O_RDONLY);
    } else {
        this->file_fd = open(PAGE_404, O_RDONLY);
        this->response = http_handler(request_).get_string();
        this->message_to_log(ERROR_404_NOT_FOUND);
    }
    this->line_.clear();

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


void ClientConnection::message_to_log(log_messages_t log_type, std::string url, std::string method) {
    switch (log_type) {
        case INFO_NEW_CONNECTION:
            this->write_to_logs("New connection [METHOD " + method + "] [URL "
                                + url
                                + "] [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET " +
                                std::to_string(this->sock)
                                + "]", INFO);
            break;
        case INFO_CONNECTION_FINISHED:
            this->write_to_logs("Connection finished successfully [WORKER PID " + std::to_string(getpid()) + "]" +
                                " [CLIENT SOCKET "
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

ClientConnection::connection_stages_t ClientConnection::process_location() {
    std::string url = request_.get_url();
    this->message_to_log(INFO_NEW_CONNECTION, url, request_.get_method());
    HttpResponse http_response;
    try {
        location_ = this->server_settings->get_location(url);
    } catch (std::exception &e) {
        return ROOT_NOT_FOUND;
    }
    if (location_->is_proxy) {
        return PASS_TO_PROXY;
    }
    return ROOT_FOUND;
}

void ClientConnection::write_to_logs(std::string message, bl::trivial::severity_level lvl) {
    for (auto &i : vector_logs) {
        i->log(message, lvl);
    }
}
