#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <ctime>
#include <exception>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include "client_connection.h"
#include "http_request.h"
#include "http_response.h"
#include "http_handle.h"

#define CLIENT_SEC_TIMEOUT 1 // maximum request idle time
#define PAGE_404 "public/404.html"
#define LENGTH_LINE_FOR_RESERVE 256
#define BUFFER_LENGTH 1024

ClientConnection::ClientConnection(class ServerSettings *server_settings,
                                   std::vector<MohicanLog *> &vector_logs) : server_settings(server_settings),
                                                                             vector_logs(vector_logs) {

}

void ClientConnection::set_socket(int socket) {
    this->sock = socket;
}

connection_status_t ClientConnection::connection_processing() {
    if (this->stage == ERROR_STAGE) {
        return ERROR_WHILE_CONNECTION_PROCESSING;
    }

    if (this->stage == GET_REQUEST) {
        bool is_succeeded;
        try {
            is_succeeded = get_request();
        } catch (std::exception &e) {
            this->stage = BAD_REQUEST;
        }
        if (is_succeeded) {
            this->stage = this->process_location();
        } else if (this->is_timeout()) {
            this->message_to_log(ERROR_TIMEOUT);
            return CONNECTION_TIMEOUT_ERROR;
        }
    }

    if (stage == ROOT_FOUND || stage == ROOT_NOT_FOUND) {
        this->make_response_header();
        this->stage = SEND_HTTP_HEADER_RESPONSE;
    }

    if (stage == PASS_TO_PROXY) {
        /* TODO: подключаемся к апстриму и передаем ему хедер запроса
         ** сокет апстрима кладем в переменную this->proxy_sock
         ** далее меняем состояние на SEND_HEADER_TO_PROXY, и возвращаем из функции состояние CONNECTION_PROXY -
         ** это значит, что в классе воркера мы должны в отслеживаемые epoll'ом сокеты вместо нашего клиентского
         ** добавить сокет апстрима, и в мапе сокет - ClientConnection подменить ключ с клиентского на апстримовский
         */
        if (connect_to_upstream()) {
            stage = SEND_HEADER_TO_PROXY;
            request_str_ = request_.get_string();
            this->message_to_log(INFO_CONNECTION_WITH_UPSTREAM, this->request_.get_url(), this->request_.get_method());
            return CHECKOUT_PROXY;
        } else {
            stage = FAILED_TO_CONNECT;
        }
    }

    if (stage == SEND_HEADER_TO_PROXY) {
        if (send_header(request_str_, proxy_sock, request_pos)) {
            this->write_to_logs("send header to proxy", INFO);
            stage = GET_BODY_OR_NOT_FROM_CLIENT;
        } else if (this->is_timeout()) {
            stage = PROXY_TIMEOUT;
        }
    }

    if (this->stage == GET_BODY_OR_NOT_FROM_CLIENT) {
        this->upstream_buffer.reserve(BUFFER_LENGTH);
        if (this->request_.get_method() == "GET") {
            this->stage = GET_RESPONSE_FROM_PROXY;
            this->write_to_logs("checkout to get response", INFO);
        } else {
            this->stage = GET_BODY_FROM_CLIENT;
            this->body_length = std::stoul(this->request_.get_headers()[CONTENT_LENGTH_HDR]);
            return CHECKOUT_CLIENT;
        }
    }

    if (stage == SEND_BODY_TO_PROXY) {
        if (this->send_body(this->proxy_sock)) {
            this->get_body_ind = 0;
            if (this->buffer_read_count >= this->body_length) {
                this->stage = GET_RESPONSE_FROM_PROXY;
                this->message_to_log(INFO_SEND_REQUEST_TO_UPSTREAM, this->request_.get_url(),
                                     this->request_.get_method());
            } else {
                this->stage = GET_BODY_FROM_CLIENT;
                return CHECKOUT_CLIENT;
            }
        } else if (this->is_timeout()) { // TODO: добавление апстрима в бан лист
            stage = PROXY_TIMEOUT;
        }
    }

    if (this->stage == GET_BODY_FROM_CLIENT) {
        this->send_body_ind = 0;
        if (this->get_body(this->sock)) {
            this->stage = SEND_BODY_TO_PROXY;
            return CHECKOUT_PROXY;
        } else if (this->is_timeout()) {
            return CONNECTION_TIMEOUT_ERROR;
        }
    }

    if (stage == GET_RESPONSE_FROM_PROXY) {
        if (get_proxy_header()) {
            response_str_ = response_.get_string();
            stage = GET_BODY_FROM_PROXY;
        } else if (this->is_timeout()) {
            this->message_to_log(ERROR_TIMEOUT);
            stage = PROXY_TIMEOUT;
        }
    }

    if (stage == GET_BODY_FROM_PROXY) {
        stage = SEND_PROXY_RESPONSE_TO_CLIENT;
        this->message_to_log(INFO_GET_RESPONSE_FROM_UPSTREAM, this->request_.get_url(), this->request_.get_method());
        return CHECKOUT_CLIENT;
    }

    if (stage == SEND_PROXY_RESPONSE_TO_CLIENT) {
        if (send_header(response_str_, sock, response_pos)) {
            this->message_to_log(INFO_SEND_UPSTREAM_RESPONSE_TO_CLIENT, this->request_.get_url(),
                                 this->request_.get_method());
            return CONNECTION_FINISHED;
        } else if (this->is_timeout()) {
            this->message_to_log(ERROR_TIMEOUT);
            return CONNECTION_TIMEOUT_ERROR;
        }
    }

    if (this->stage == SEND_HTTP_HEADER_RESPONSE) {
        if (this->send_header(response_str_, sock, response_pos)) {
            this->stage = SEND_FILE;
        } else if (this->is_timeout()) {
            this->message_to_log(ERROR_TIMEOUT);
            return CONNECTION_TIMEOUT_ERROR;
        }
    }

    if (this->stage == SEND_FILE) {
        if (this->send_file()) {
            this->message_to_log(INFO_CONNECTION_FINISHED);
            return CONNECTION_FINISHED;
        } else if (this->is_timeout()) {
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

bool ClientConnection::make_response_header() {
    if (stage == ROOT_FOUND) {
        response_ = http_handler(request_, location_->root);
        this->response_str_ = response_.get_string();
        this->file_fd = open((location_->root + request_.get_url()).c_str(), O_RDONLY);
        if (this->file_fd == -1) {
            this->stage = ROOT_NOT_FOUND;
        }
    }
    if (this->stage == ROOT_NOT_FOUND) {
        this->file_fd = open(PAGE_404, O_RDONLY);
        this->response_ = http_handler(request_);
        this->response_str_ = response_.get_string();
        this->message_to_log(ERROR_404_NOT_FOUND);
    }
    this->line_.clear();

    return true;
}

bool ClientConnection::send_header(std::string &str, int socket, int &pos) {
    bool is_write_data = false;
    int write_result;
    while ((write_result = write(socket, str.c_str() + pos, 1)) == 1) {
        pos++;
        if (pos == str.size() - 1) {
            str.clear();
            write_result = write(socket, "\r\n", 2);
            if (write_result == -1) {
                this->stage = ERROR_STAGE;
                this->write_to_logs("error = " + std::string(strerror(errno)), WARNING);
                this->message_to_log(ERROR_SEND_RESPONSE);
                return false;
            }
            return true;
        }
        is_write_data = true;
    }
    if (write_result == -1) {
        this->stage = ERROR_STAGE;
        this->write_to_logs("error = " + std::string(strerror(errno)), WARNING);
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
    while (read_code > 0 && (write_result = write(this->sock, &c, sizeof(c)) == sizeof(c))) {
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

void ClientConnection::message_to_log(log_messages_t log_type) {
    switch (log_type) {
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
            /*this->write_to_logs("TIMEOUT ERROR [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET "
                                + std::to_string(this->sock) + "]", ERROR);*/
            break;
        case ERROR_READING_REQUEST:
            this->write_to_logs("Reading request error [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET "
                                + std::to_string(this->sock) + "]", ERROR);
            break;
        case ERROR_SEND_RESPONSE:
            this->write_to_logs("Send response_str_ error [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET "
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

void ClientConnection::message_to_log(log_messages_t log_type, std::string &url, std::string &method) {
    switch (log_type) {
        case INFO_NEW_CONNECTION:
            this->write_to_logs("New connection [METHOD " + method + "] [URL "
                                + url
                                + "] [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET " +
                                std::to_string(this->sock)
                                + "]", INFO);
            break;
        case INFO_CONNECTION_WITH_UPSTREAM:
            this->write_to_logs(
                    "CONNECT TO UPSTREAM [UPSTREAM " + this->location_->upstreams[0]->get_upstream_address() + "] [URL "
                    + url
                    + "] [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET " +
                    std::to_string(this->sock)
                    + "]", INFO);
            break;
        case INFO_SEND_REQUEST_TO_UPSTREAM:
            this->write_to_logs(
                    "SEND REQUEST TO UPSTREAM [UPSTREAM " + this->location_->upstreams[0]->get_upstream_address() +
                    "] [URL " + url + "] [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET " +
                    std::to_string(this->sock) + "]", INFO);
            break;
        case INFO_GET_RESPONSE_FROM_UPSTREAM:
            this->write_to_logs(
                    "GET RESPONSE FROM UPSTREAM [UPSTREAM " + this->location_->upstreams[0]->get_upstream_address() +
                    "] [URL " + url + "] [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET " +
                    std::to_string(this->sock) + "]", INFO);
            break;
        case INFO_SEND_UPSTREAM_RESPONSE_TO_CLIENT:
            this->write_to_logs(
                    "SEND UPSTREAM RESPONSE TO CLIENT [UPSTREAM " +
                    this->location_->upstreams[0]->get_upstream_address() +
                    "] [URL " + url + "] [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET " +
                    std::to_string(this->sock) + "]", INFO);
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

int &ClientConnection::get_upstream_sock() {
    return this->proxy_sock;
}

int ClientConnection::get_client_sock() {
    return this->sock;
}

bool ClientConnection::connect_to_upstream() {
    this->write_to_logs("connect to upstream start", INFO);
    if ((get_upstream_sock() = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return false;
    }
    this->write_to_logs("connect to upstream do 1", INFO);
    UpstreamSettings *upstream = location_->upstreams[0];
    this->write_to_logs("connect to upstream do 1.1", INFO);
    this->write_to_logs(std::to_string(upstream->get_port()), INFO);
    if (!(upstream->is_ip_address())) {
        this->write_to_logs("connect to upstream do 1.1.1", INFO);
        struct sockaddr_in *serv_addr;
        struct addrinfo *result = NULL;
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        if (getaddrinfo(upstream->get_upstream_address().c_str(), NULL, &hints, &result)) {
            return false;
        }
        serv_addr = (struct sockaddr_in *) result->ai_addr;
        serv_addr->sin_family = AF_INET;
        serv_addr->sin_port = htons(upstream->get_port());
        if (connect(get_upstream_sock(), (struct sockaddr *) serv_addr, sizeof(*serv_addr)) < 0) {
            return false;
        }
    } else {
        this->write_to_logs("connect to upstream do 1.2", INFO);
        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(upstream->get_port());
        this->write_to_logs("connect to upstream do 2", INFO);
        if (inet_pton(AF_INET, upstream->get_upstream_address().c_str(), &serv_addr.sin_addr) <= 0) {
            return false;
        }
        this->write_to_logs("connect to upstream do 3", INFO);
        if (connect(get_upstream_sock(), (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            return false;
        }
        this->write_to_logs("connect to upstream do 4", INFO);
    }
    fcntl(get_upstream_sock(), F_SETFL, fcntl(get_upstream_sock(), F_GETFL, 0) | O_NONBLOCK);
    this->write_to_logs("connect to upstream do 5", INFO);
    return true;
}

bool ClientConnection::get_proxy_header() {
    bool is_read_data = false;
    int result_read;
    this->write_to_logs("we are in get_proxy_header", INFO);
    while ((result_read = read(proxy_sock, &last_char_, sizeof(last_char_))) == sizeof(last_char_)) {
        this->upstream_buffer.push_back(last_char_);
        if (this->last_char_ == '\n') {
            this->response_.add_line(line_);
            this->upstream_buffer.clear();
            this->upstream_buffer.reserve(BUFFER_LENGTH);
        }
        is_read_data = true;
    }

    if (response_.response_ended()) {
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

bool ClientConnection::get_body(int socket) {
    bool is_read_data = false;
    int result_read;
    this->write_to_logs("we are in get_body(!!)", INFO);
    while (this->get_body_ind < BUFFER_LENGTH &&
           (result_read = read(socket, &last_char_, sizeof(last_char_))) == sizeof(last_char_)) {
        this->upstream_buffer.push_back(this->last_char_);
        this->get_body_ind++;
        this->buffer_read_count++;
        this->write_to_logs("get " + std::to_string(this->get_body_ind) + " bite from proxy", INFO);
        is_read_data = true;
    }

    if (result_read == -1) {
        this->stage = ERROR_STAGE;
        return false;
    }

    if (is_read_data) {
        this->timeout = clock() / CLOCKS_PER_SEC;
    }

    if (this->get_body_ind >= BUFFER_LENGTH || this->buffer_read_count >= this->body_length) {
        return true;
    }

    return false;
}


bool ClientConnection::is_timeout() {
    return clock() / CLOCKS_PER_SEC - this->timeout > CLIENT_SEC_TIMEOUT;
}

bool ClientConnection::send_body(int socket) {
    bool is_write_data = false;
    int write_result;
    while ((write_result = write(socket, this->upstream_buffer.c_str() + this->send_body_ind, 1)) ==
           1) {
        this->send_body_ind++;
        this->write_to_logs("send " + std::to_string(this->send_body_ind) + " bite to proxy", INFO);
        if (this->send_body_ind == this->get_body_ind) {
            break;
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

    if (this->send_body_ind == this->get_body_ind) {
        return true;
    }

    return false;
}
