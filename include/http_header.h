#pragma once

#include <string>

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
