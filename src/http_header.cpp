#include "http_header.h"

HttpHeader::HttpHeader(std::string &header, std::string &value) : header_(header), value_(value) {}

void HttpHeader::set_header(std::string &header, std::string &value) {
    header_ = header;
    value_ = value;
}

std::string &HttpHeader::get_header() {
    return header_;
}

std::string HttpHeader::get_header() const {
    return std::string(header_);
}

std::string &HttpHeader::get_value() {
    return value_;
}

std::string HttpHeader::get_value() const {
    return std::string(value_);
}
