#include "http_header.h"

HttpHeader::HttpHeader(std::string &header, std::string &value) : header_(header), value_(value) {}

void HttpHeader::setHeader(std::string &header, std::string &value) {
    header_ = header;
    value_ = value;
}

std::string &HttpHeader::getHeader() {
    return header_;
}

std::string HttpHeader::getHeader() const {
    return std::string(header_);
}

std::string &HttpHeader::getValue() {
    return value_;
}

std::string HttpHeader::getValue() const {
    return std::string(value_);
}
