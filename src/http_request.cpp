#include <unistd.h>
#include <cstring>

#include <http_exceptions.h>
#include "http_request.h"

std::string &HttpRequest::get_method() {
    return method_;
}

std::string HttpRequest::get_method() const {
    return std::string(method_);
}

void HttpRequest::set_method(const std::string &method) {
    method_ = method;
}

HttpRequest::HttpRequest(const int fd) {
    char buffer[buf_size_];
    int buffer_len = read_line(fd, buffer);
    if (buffer_len == -1) {
        throw ReadException("Error while reading from file descriptor");
    }

    char *method_begin = buffer;
    char *method_end = strchr(method_begin, ' ');
    if (!method_end) {
        throw DelimException("Space not found");
    }

    method_ = std::string(method_begin, method_end - method_begin);

    char *url_begin = method_end + 1;
    while (isspace(*url_begin))
        url_begin++;
    char *url_end = strchr(url_begin, ' ');
    if (!url_end) {
        throw DelimException("Space not found");
    }

    url_ = std::string(url_begin, url_end - url_begin);

    char *protocol_begin = url_end + 1;

    if (sscanf(protocol_begin, "HTTP/%d.%d", &version_major_, &version_minor_) != 2) {
        throw ReadException("Error while reading from file descriptor");
    }

    if (version_major_ != 1 || (version_minor_ != 0 && version_minor_ != 1)) {
        throw ProtVersionException("Unsupported http version");
    }

    while ((buffer_len = read_line(fd, buffer)) > 0) {
        char *header_name_begin = buffer;
        char *header_name_end = strchr(header_name_begin, ':');
        if (!header_name_end)
            continue;

        char *header_value_begin = header_name_end + 1;
        while (isspace(*header_value_begin))
            header_value_begin++;
        char *header_value_end = buffer + buffer_len;

        std::string header_name = std::string(header_name_begin, header_name_end - header_name_begin);
        std::string header_value = std::string(header_value_begin, header_value_end - header_value_begin);
        request_headers_.push_back(HttpHeader(header_name, header_value));
    }
}

int HttpRequest::read_line(const int fd, char *buffer) {
    int i = 0;
    while (i < buf_size_) {
        char c;
        int r = read(fd, &c, sizeof c);
        if (r <= 0)
            return -1;
        if (c == '\n')
            break;
        buffer[i++] = c;
    }
    if (i > 0 && buffer[i - 1] == '\r')
        i--;
    buffer[i] = '\0';
    return i;
}

