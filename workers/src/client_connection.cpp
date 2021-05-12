#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <exception>
#include <fstream>
#include <iostream>


#include "client_connection.h"
#include "http_request.h"
#include "http_response.h"
#include "http_handle.h"

#define MAX_METHOD_LENGTH 4
#define CLIENT_SEC_TIMEOUT 5 // maximum request idle time
#define PAGE_404 "404.html" // потом изменить
#define FOR_404_RESPONSE "/sadfsadf/sadfsaf/asdfsaddf"

ClientConnection::ClientConnection(int sock, class ServerSettings *server_settings) : sock(sock), server_settings(
        server_settings) {}

connection_status_t ClientConnection::connection_processing() {
    if (this->stage == GET_REQUEST) {
        if (this->get_request()) {
            this->stage = FORM_HTTP_HEADER_RESPONSE;
            std::cout << this->request << std::endl;
        } else if (clock() / CLOCKS_PER_SEC - this->timeout > CLIENT_SEC_TIMEOUT) {
            // if the user does not send data for a long time, we close the connection
            close(this->sock);
            return CONNECTION_TIMEOUT_ERROR;

        }
    }

    if (this->stage == FORM_HTTP_HEADER_RESPONSE) {
        if (this->form_http_header_response()) {
            this->stage = SEND_HTTP_HEADER_RESPONSE;
        }
        std::cout << "privetic" << std::endl;
    }

    if (this->stage == SEND_HTTP_HEADER_RESPONSE) {
        if (this->send_http_header_response()) {
            this->stage = SEND_FILE;
        } else if (clock() / CLOCKS_PER_SEC - this->timeout > CLIENT_SEC_TIMEOUT) {
            // if the user cannot accept the data for a long time we close the connection
            close(this->sock);
            return CONNECTION_TIMEOUT_ERROR;
        }
    }

    if (this->stage == SEND_FILE) {
        if (this->send_file()) {
            this->close_connection();
            return CONNECTION_FINISHED;
        } else if (clock() / CLOCKS_PER_SEC - this->timeout > CLIENT_SEC_TIMEOUT) {
            close(this->sock);
            return CONNECTION_TIMEOUT_ERROR;
        }
    }

    return CONNECTION_PROCESSING;
}

bool ClientConnection::get_request() {
    char c;
    bool is_read_data = false;
    while (read(this->sock, &c, sizeof(c)) == sizeof(c)) {
        this->request.push_back(c);
        if (this->request.size() >= MAX_METHOD_LENGTH) {
            this->set_method();
        }
        is_read_data = true;
    }

    if (this->is_end_request()) {
        return true;
    }

    if (is_read_data) {
        this->timeout = clock() / CLOCKS_PER_SEC;
    }

    return false;
}


bool ClientConnection::form_http_header_response() {
    HttpRequest http_request(this->request);
    std::string url = http_request.get_url();
    HttpResponse http_response;
    try {
        std::string root = this->server_settings->get_root(url);
        http_response = http_handler(http_request, root);
        this->file_fd = open((root + url).c_str(), O_RDONLY);
        if (this->file_fd == -1) {
            this->file_fd = open(PAGE_404, O_RDONLY);
        }
    } catch (std::exception &e) {
        // добавить запись в error log
        http_response = http_handler(http_request);
        this->file_fd = open(PAGE_404, O_RDONLY); // добавить дефолтную 404 html страничку
    }
    this->response = http_response.get_string();
    this->request.clear();


    return true;
}


bool ClientConnection::send_http_header_response() {
    bool is_write_data = false;
    while (write(this->sock, this->response.c_str() + response_pos, 1) == 1) {
        response_pos++;
        if (response_pos == this->response.size() - 1) {
            this->response.clear();
            return true;
        }
        is_write_data = true;
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

    read_code = read(this->file_fd, &c, sizeof(c));
    while (write(this->sock, &c, sizeof(c)) == sizeof(c) && read_code > 0) {
        read_code = read(this->file_fd, &c, sizeof(c));
        is_write_data = true;
    }

    if (read_code == 0) {
        return true;
    }

    if (is_write_data) {
        this->timeout = clock() / CLOCKS_PER_SEC;
    }

    return false;
}


void ClientConnection::close_connection() {
    close(this->sock);
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