#pragma once

#include <string>
#include <vector>

class HttpHeader {
 public:
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
    HttpBase(std::vector<HttpHeader> &headers, int major = 0, int minor = 0);
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
    std::string &getMethod();
    std::string getMethod() const;
    void setMethod(const std::string &method);
 private:
    std::string http_method_;
    std::string data_;
};
