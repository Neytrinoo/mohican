#pragma once

#include <string>
#include <vector>

#include "http_base.h"

class HttpRequest : public HttpBase {
 public:
    HttpRequest() = default;
    HttpRequest(std::string &method, std::string &data, std::vector<HttpHeader> &headers, int major = 1, int minor = 0);
    HttpRequest(const char *file_as_string);
    HttpRequest(const int fd);
    std::string &getMethod();
    std::string getMethod() const;
    void setMethod(const std::string &method);
 private:
    std::string http_method_;
    std::string http_data_;
};
