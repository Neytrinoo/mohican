#include <unistd.h>
#include <cstring>

#include "http_parse.h"

std::string &HttpRequest::get_method() {
    return method_;
}

std::string HttpRequest::get_method() const {
    return std::string(method_);
}

void HttpRequest::set_method(const std::string &method) {
    method_ = method;
}

HttpRequest::HttpRequest(std::string &method, std::string &data, std::vector<HttpHeader> &headers, int major, int minor)
        : method_(method), data_(data), HttpBase(headers, major, minor) {}

HttpRequest::HttpRequest(const int fd) {
    char buffer[buf_size_];
    int buffer_len = read_line(fd, buffer);

    char *method_begin = buffer;
    char *method_end = strchr(method_begin, ' ');

    method_ = std::string(method_begin, method_end - method_begin);

    char *url_begin = method_end + 1;
    while (isspace(*url_begin))
        url_begin++;
    char *url_end = strchr(url_begin, ' ');

    url_ = std::string(url_begin, url_end - url_begin);

    char *protocol_begin = url_end + 1;
    while (isspace(*protocol_begin))
        protocol_begin++;
    char *protocol_end = buffer + buffer_len;

    protocol_ = std::string(protocol_begin, protocol_end - protocol_begin);

    while ((buffer_len = read_line(fd, buffer)) != 0) {
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
        http_headers_.push_back(HttpHeader(header_name, header_value));
    }
    while (read_body(fd, buffer) > 0) {
        data_ += std::string(buffer);
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

int HttpRequest::read_body(const int fd, char *buffer) {
    int r = read(fd, buffer, buf_size_);
    if (r <= 0)
        return -1;
    buffer[r] = '\0';
    return r;
}
