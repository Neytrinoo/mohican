#include <unistd.h>

#include "defines.h"
#include "http_response.h"

HttpResponse::HttpResponse(std::unordered_map<std::string, std::string> headers,
                           int major,
                           int minor,
                           const std::string &filename,
                           int status,
                           const std::string &message,
                           int file_fd,
                           int method)
        : HttpBase(headers, major, minor),
          filename_(filename),
          status_(status),
          message_(message),
          file_fd_(file_fd),
          method_(method) {}

int HttpResponse::send(int sock) {
    if (send_status(sock) != OK) {
        return NOTOK;
    }
    if (send_nl(sock) != OK) {
        return NOTOK;
    }
    if (send_headers(sock) != OK) {
        return NOTOK;
    }
    if (send_nl(sock) != OK) {
        return NOTOK;
    }
    if (method_ == GET) {
        if (send_file(sock) != OK) {
            return NOTOK;
        }
    }
    return OK;
}

int HttpResponse::send_nl(int sock) {
    return write(sock, "\n", 1);
}

int HttpResponse::send_status(int sock) {
    std::string line = "HTTP/" + std::to_string(get_major()) + "." +
            std::to_string(get_minor()) + " " + std::to_string(status_) + message_;
    return write(sock, line.c_str(), line.size());
}

int HttpResponse::send_headers(int sock) {
    int symbols = 0;
    for (const auto &header: headers_) {
        std::string line = header.first + ": " + header.second;
        symbols += write(sock, line.c_str(), line.size());
        symbols += send_nl(sock);
    }
    return symbols;
}

int HttpResponse::send_file(int sock) {
    if (file_fd_ == NOFILE) {
        return NOFILE;
    }
    char buffer[buf_size_];
    while (true) {
        int r = read(file_fd_, buffer, sizeof(buffer));
        if (r < 0)
            return NOTOK;
        if (r == 0)
            break;
        if (write(sock, buffer, r) == NOTOK)
            return NOTOK;
    }
    close(file_fd_);
    return OK;
}
