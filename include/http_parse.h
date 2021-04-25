#pragma once

#include <string>
#include <vector>

class HttpHeader {
 public:
    HttpHeader() = default;
    HttpHeader(std::string &header, std::string &value);
    ~HttpHeader() = default;
    void setHeader(std::string &header, std::string &value);
    std::string &getHeader();
    std::string getHeader() const;
    std::string &getValue();
    std::string getValue() const;
 private:
    std::string header_;
    std::string value_;
};

class HttpBase {
 public:
    HttpBase();
    HttpBase(std::vector<HttpHeader> &headers, int major = 1, int minor = 0);
    ~HttpBase() = default;
    void setVersion(int major, int minor);
    void setHeaders(std::vector<HttpHeader> headers);
    int &getMinor();
    int getMinor() const;
    int &getMajor();
    int getMajor() const;
 protected:
    int http_version_major_;
    int http_version_minor_;
    std::vector<HttpHeader> http_headers_;
};

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
