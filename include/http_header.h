#pragma once

#include <string>

class HttpHeader {
 public:
    HttpHeader() = default;
    HttpHeader(std::string &header, std::string &value);
    ~HttpHeader() = default;
    void set_header(std::string &header, std::string &value);
    std::string &get_header();
    std::string get_header() const;
    std::string &get_value();
    std::string get_value() const;
 private:
    std::string header_;
    std::string value_;
};
