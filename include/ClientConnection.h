#pragma once

#include <string>
#include <ctime>

#include "config_parse.h"


typedef enum {
    CONNECTION_PROCESSING,
    CONNECTION_TIMEOUT_ERROR,
    CONNECTION_FINISHED
} connection_status_t;

class ClientConnection {
public:
    ClientConnection(int sock, class ServerSettings *server_settings);

    connection_status_t connection_processing();

private:
    int sock;
    clock_t timeout;

    typedef enum {
        GET_REQUEST,
        FORM_HTTP_HEADER_RESPONSE,
        SEND_HTTP_HEADER_RESPONSE,
        SEND_FILE
    } connection_stages_t;

    typedef enum {
        NONE,
        GET,
        POST,
    } methods_t;

    connection_stages_t stage = GET_REQUEST;

    methods_t method = NONE;

    class ServerSettings *server_settings;

    std::string request;
    std::string response;

    int response_pos = 0;
    int file_fd;

    // return true if their connection processing stage is finished
    bool get_request();

    bool form_http_header_response();

    bool send_http_header_response();

    bool send_file();

    bool is_end_request();  // check if the request has ended

    void set_method();

    void close_connection();
};
