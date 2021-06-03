#pragma once

#include <string>
#include <ctime>

#include "http_request.h"
#include "http_response.h"
#include "server_settings.h"
#include "mohican_log.h"
#include "define_log.h"

#define MAX_FILE_READ 100

typedef enum {
    CONNECTION_PROCESSING,
    CONNECTION_PROXY,
    CONNECTION_TIMEOUT_ERROR,
    CONNECTION_FINISHED,
    ERROR_WHILE_CONNECTION_PROCESSING,
    CHECKOUT_CLIENT_FOR_READ,
    CHECKOUT_CLIENT_FOR_WRITE,
    CHECKOUT_PROXY_FOR_READ,
    CHECKOUT_PROXY_FOR_WRITE,
} connection_status_t;

class ClientConnection {
public:
    ClientConnection(class ServerSettings *server_settings, std::vector<MohicanLog*>& vector_logs);

    ClientConnection &operator=(const ClientConnection &other) = default;

    ClientConnection(const ClientConnection &other) = default;

    ClientConnection() = default;

    connection_status_t connection_processing();

    clock_t get_timeout();

    int &get_upstream_sock();

    int get_client_sock();

    void write_to_logs(std::string message, bl::trivial::severity_level lvl);

    void set_socket(int socket);
private:
    int sock;
    int proxy_sock;
    clock_t timeout;

    std::vector<MohicanLog*> vector_logs;

    typedef enum {
        GET_REQUEST,
        ROOT_FOUND,
        ROOT_NOT_FOUND,
        SEND_HTTP_HEADER_RESPONSE,
        SEND_FILE,
        BAD_REQUEST,
        PASS_TO_PROXY,
        SEND_HEADER_TO_PROXY,
        GET_BODY_FROM_CLIENT,
        GET_BODY_FROM_PROXY,
        SEND_BODY_TO_PROXY,
        SEND_PROXY_RESPONSE_TO_CLIENT,
        GET_BODY_OR_NOT_FROM_CLIENT,
        SEND_BODY_TO_CLIENT,
        FAILED_TO_CONNECT,
        PROXY_TIMEOUT,
        GET_RESPONSE_FROM_PROXY,
        ERROR_STAGE
    } connection_stages_t;

    typedef enum {
        NONE,
        GET,
        POST,
    } methods_t;

    typedef enum {
        INFO_NEW_CONNECTION,
        INFO_CONNECTION_FINISHED,
        INFO_CONNECTION_WITH_UPSTREAM,
        INFO_SEND_REQUEST_TO_UPSTREAM,
        INFO_GET_RESPONSE_FROM_UPSTREAM,
        INFO_SEND_UPSTREAM_RESPONSE_TO_CLIENT,
        ERROR_404_NOT_FOUND,
        ERROR_TIMEOUT,
        ERROR_READING_REQUEST,
        ERROR_SEND_RESPONSE,
        ERROR_SEND_FILE,
        ERROR_BAD_REQUEST,
    } log_messages_t;

    connection_stages_t stage = GET_REQUEST;

    methods_t method = NONE;

    class ServerSettings *server_settings;

    char last_char_;
    std::string line_;
    HttpRequest request_;
    location_t *location_ = nullptr;
    std::string response_str_;
    std::string request_str_;

    std::string upstream_buffer; // буфер для отправки запросов и ответов между клиентом и апстримом
    int buffer_read_count = 0; // количество считанных данных с клиента или с апстрима всего
    int get_body_ind = 0; // количество (индекс) считанных данных за одну стадию (если не помещается в буфер)
    size_t body_length; // длина тела пользовательского запроса
    int send_body_ind = 0; // количество (индекс) отправленных байтов тела запроса клиента апстриму

    HttpResponse response_;

    int request_pos = 0;
    int response_pos = 0;
    int file_fd;
    //int write_result = 0;
    //char file_bytes[MAX_FILE_READ];

    // return true if their connection processing stage is finished
    bool get_request();
    connection_stages_t process_location();
    bool make_response_header();

    bool send_header(std::string& str, int socket, int& pos);

    bool send_file();

    bool get_body(int socket);

    void message_to_log(log_messages_t log_type, std::string &url, std::string &method);

    void message_to_log(log_messages_t log_type);

    bool connect_to_upstream();

    bool is_timeout();

    bool send_body(int socket);
    bool get_proxy_header();
};
