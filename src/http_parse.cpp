#include "http_parse.h"

std::string &HttpRequest::get_method() {
    return http_method_;
}

std::string HttpRequest::get_method() const {
    return std::string(http_method_);
}

void HttpRequest::setMethod(const std::string &method) {
    http_method_ = method;
}

HttpRequest::HttpRequest(std::string &method, std::string &data, std::vector<HttpHeader> &headers, int major, int minor)
        : http_method_(method), http_data_(data), HttpBase(headers, major, minor) {}

HttpRequest::HttpRequest(const char *file_as_string) {
    char buffer[buf_size_];

}

//HttpRequest::HttpRequest(const int fd);
