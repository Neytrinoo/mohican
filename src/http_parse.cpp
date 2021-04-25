#include "http_parse.h"

#include <utility>

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

HttpBase::HttpBase(std::vector<HttpHeader> &headers, int major, int minor)
        : http_headers_(headers), http_version_major_(major), http_version_minor_(minor) {}

void HttpBase::setVersion(int major, int minor) {
    http_version_major_ = major;
    http_version_minor_ = minor;
}

void HttpBase::setHeaders(std::vector<HttpHeader> headers) {
    http_headers_ = std::move(headers);
}

int &HttpBase::getMinor() {
    return http_version_minor_;
}

int HttpBase::getMinor() const {
    return http_version_minor_;
}

int &HttpBase::getMajor() {
    return http_version_major_;
}

int HttpBase::getMajor() const {
    return http_version_major_;
}

std::string &HttpRequest::getMethod() {
    return http_method_;
}

std::string HttpRequest::getMethod() const {
    return std::string(http_method_);
}

void HttpRequest::setMethod(const std::string &method) {
    http_method_ = method;
}
