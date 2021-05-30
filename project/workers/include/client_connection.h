#pragma once

#include <string>
#include <ctime>

#include "http_request.h"
#include "server_settings.h"
#include "mohican_log.h"
#include "define_log.h"

typedef enum {
    CONNECTION_PROCESSING,
    CONNECTION_PROXY,
    CONNECTION_TIMEOUT_ERROR,
    CONNECTION_FINISHED,
    ERROR_WHILE_CONNECTION_PROCESSING,
} connection_status_t;

class ClientConnection {
public:
    ClientConnection(int sock, class ServerSettings *server_settings, std::vector<MohicanLog*>& vector_logs);

    ClientConnection &operator=(const ClientConnection &other) = default;

    ClientConnection(const ClientConnection &other) = default;

    ClientConnection() = default;

    connection_status_t connection_processing();

    clock_t get_timeout();

    int get_upstream_sock();

    int get_client_sock();

    void write_to_logs(std::string message, bl::trivial::severity_level lvl);
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
        READ_BODY_FROM_CLIENT,
        SEND_BODY_TO_PROXY,
        SENT_PROXY_RESPONSE_TO_CLIENT,
        FAILED_TO_CONNECT,
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
        ERROR_404_NOT_FOUND,
        ERROR_TIMEOUT,
        ERROR_READING_REQUEST,
        ERROR_SEND_RESPONSE,
        ERROR_SEND_FILE,
        ERROR_BAD_REQUEST
    } log_messages_t;

    connection_stages_t stage = GET_REQUEST;

    methods_t method = NONE;

    class ServerSettings *server_settings;

    char last_char_;
    std::string line_;
    HttpRequest request_;
    location_t *location_ = nullptr;
    std::string response;

    int response_pos = 0;
    int file_fd;

    // return true if their connection processing stage is finished
    bool get_request();
    connection_stages_t process_location();
    bool make_response_header();

    bool send_http_header_response();

    bool send_file();

    bool is_end_request();  // check if the line_ has ended

    void set_method();

    void message_to_log(log_messages_t log_type, std::string url = "", std::string method = "");

    bool connect_to_upstream();
};
