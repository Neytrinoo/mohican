#pragma once

#include <string>
#include <unordered_map>

#include "http_base.h"
#include "http_request.h"

class HttpResponse : public HttpBase {
 public:
    HttpResponse() = default;
    HttpResponse(std::unordered_map<std::string, std::string> headers,
                 int major,
                 int minor,
                 const std::string &filename,
                 int status,
                 const std::string &message,
                 int file_fd,
                 int method);
    HttpResponse(const HttpResponse &other) = default;
    HttpResponse &operator=(const HttpResponse &other) = default;
    HttpResponse(HttpResponse &&other) = default;
    HttpResponse &operator=(HttpResponse &&other) = default;

    int send(int sock);

 private:
    int status_ = 0;
    std::string message_;
    std::string filename_;
    int file_fd_;
    int method_;

 private:
    int send_nl(int sock);
    int send_status(int sock);
    int send_headers(int sock);
    int send_file(int sock);
};