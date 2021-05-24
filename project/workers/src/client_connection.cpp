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

ClientConnection::ClientConnection(int sock, class ServerSettings* server_settings) : sock(sock), server_settings(
        server_settings) {}

connection_status_t ClientConnection::connection_processing() {
    if (this->stage == ERROR) {
        return ERROR_WHILE_CONNECTION_PROCESSING;
    }

    if (this->stage == GET_REQUEST) {
        if (!get_request() && clock() / CLOCKS_PER_SEC - this->timeout > CLIENT_SEC_TIMEOUT) {
            this->write_to_log(ERROR_TIMEOUT);
            return CONNECTION_TIMEOUT_ERROR;
        }
        try {
            if (last_char_ == '\n') {
                request_.add_line(line_);
                line_.clear();
            }
        } catch (std::exception& e) {
            this->write_to_log(ERROR_BAD_REQUEST);
            // TODO: добавить обработку BAD REQUEST запроса (какой-то дефолтный ответ и страничка)
            stage = BAD_REQUEST;
        }
        if (request_.first_line_added()) {
            stage = process_location();
        }
    }

    if (stage == ROOT_FOUND) {
        make_response_header(true);
        stage = SEND_HTTP_HEADER_RESPONSE;
    }
    if (stage == ROOT_NOT_FOUND) {
        make_response_header(false);
        stage = SEND_HTTP_HEADER_RESPONSE;

    }

    if (this->stage == SEND_HTTP_HEADER_RESPONSE) {
        if (this->send_http_header_response()) {
            this->stage = SEND_FILE;
        } else if (clock() / CLOCKS_PER_SEC - this->timeout > CLIENT_SEC_TIMEOUT) {
            this->write_to_log(ERROR_TIMEOUT);
            return CONNECTION_TIMEOUT_ERROR;
        }
    }

    if (this->stage == SEND_FILE) {
        if (this->send_file()) {
            this->write_to_log(INFO_CONNECTION_FINISHED);
            return CONNECTION_FINISHED;
        } else if (clock() / CLOCKS_PER_SEC - this->timeout > CLIENT_SEC_TIMEOUT) {
            this->write_to_log(ERROR_TIMEOUT);
            return CONNECTION_TIMEOUT_ERROR;
        }
    }

    return CONNECTION_PROCESSING;
}

bool ClientConnection::get_request() {
    bool is_read_data = false;
    int result_read;
    while ((result_read = read(this->sock, &last_char_, sizeof(last_char_))) == sizeof(last_char_)) {
        this->line_.push_back(last_char_);
        if (this->line_.size() >= MAX_METHOD_LENGTH) {
            this->set_method();
        }
        is_read_data = true;
    }

    if (request_.requst_ended()) {
        return true;
    }

    if (result_read == -1) {
        this->stage = ERROR;
        return false;
    }

    if (is_read_data) {
        this->timeout = clock() / CLOCKS_PER_SEC;
    }

    return false;
}

bool ClientConnection::make_response_header(bool root_found) {
    if (root_found) {
        this->response = http_handler(request_, location_.root).get_string();
        this->file_fd = open((location_.root + request_.get_url()).c_str(), O_RDONLY);
    } else {
        this->file_fd = open(PAGE_404, O_RDONLY);
        this->response = http_handler(request_).get_string();
        this->write_to_log(ERROR_404_NOT_FOUND);
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
                this->stage = ERROR;
                this->write_to_log(ERROR_SEND_RESPONSE);
                return false;
            }
            return true;
        }
        is_write_data = true;
    }

    if (write_result == -1) {
        this->stage = ERROR;
        this->write_to_log(ERROR_SEND_RESPONSE);
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
        this->stage = ERROR;
        this->write_to_log(ERROR_SEND_FILE);
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
    if (this->line_.substr(0, 3) == "GET") {
        this->method = GET;
    } else if (this->line_.substr(0, 4) == "POST") {
        this->method = POST;
    }
}

bool ClientConnection::is_end_request() {
    size_t pos = this->line_.find("\r\n\r\n");
    return pos != std::string::npos;
}

void ClientConnection::write_to_log(log_messages_t log_type, std::string url, std::string method) {
    switch (log_type) {
        case INFO_NEW_CONNECTION:
            BOOST_LOG_TRIVIAL(info) << "New connection [METHOD " << method << "] [URL "
                                    << url
                                    << "] [WORKER PID " << getpid() << "] [CLIENT SOCKET " << this->sock
                                    << "]";
            break;
        case INFO_CONNECTION_FINISHED:
            BOOST_LOG_TRIVIAL(info) << "Connection finished successfully [WORKER PID " << getpid() << "]"
                                    << " [CLIENT SOCKET "
                                    << this->sock << "]";
            break;
        case ERROR_404_NOT_FOUND:
            BOOST_LOG_TRIVIAL(error) << "404 NOT FOUND [WORKER PID " << getpid() << "] [CLIENT SOCKET "
                                     << this->sock << "]";
            break;
        case ERROR_TIMEOUT:
            BOOST_LOG_TRIVIAL(error) << "TIMEOUT ERROR [WORKER PID " << getpid() << "] [CLIENT SOCKET "
                                     << this->sock << "]";
            break;
        case ERROR_READING_REQUEST:
            BOOST_LOG_TRIVIAL(error) << "Reading request error [WORKER PID " << getpid() << "] [CLIENT SOCKET "
                                     << this->sock << "]";
            break;
        case ERROR_SEND_RESPONSE:
            BOOST_LOG_TRIVIAL(error) << "Send response error [WORKER PID " << getpid() << "] [CLIENT SOCKET "
                                     << this->sock << "]";
            break;
        case ERROR_SEND_FILE:
            BOOST_LOG_TRIVIAL(error) << "Send response error [WORKER PID " << getpid() << "] [CLIENT SOCKET "
                                     << this->sock << "]";
            break;
        case ERROR_BAD_REQUEST:
            BOOST_LOG_TRIVIAL(error) << "Bad request error [WORKER PID " << getpid() << "] [CLIENT SOCKET "
                                     << this->sock << "]";
            break;
    }
}

clock_t ClientConnection::get_timeout() {
    return this->timeout;
}

ClientConnection::connection_stages_t ClientConnection::process_location() {
    std::string url = request_.get_url();
    this->write_to_log(INFO_NEW_CONNECTION, url, request_.get_method());
    HttpResponse http_response;
    try {
        location_ = this->server_settings->get_location(url);
    } catch (std::exception& e) {
        this->write_to_log(ERROR_404_NOT_FOUND);
        return ROOT_NOT_FOUND;
    }
    if (location_.is_proxy) {
        return PASS_TO_PROXY;
    }
    return ROOT_FOUND;
}
