#include "http_parse.h"

#include <utility>

HttpHeader::HttpHeader(std::string &header, std::string &value) : header_(header), value_(value) {}

HttpBase::HttpBase(std::vector<HttpHeader> &headers, int major, int minor)
        : http_headers_(headers), http_version_major_(major), http_version_minor_(minor) {}

void HttpBase::setVersion(int major, int minor) {
    http_version_major_ = major;
    http_version_minor_ = minor;
}

void HttpBase::setHeaders(std::vector<HttpHeader> headers) {
    http_headers_ = std::move(headers);
}

int HttpBase::getMinor() const {
    return http_version_minor_;
}

int HttpBase::getMajor() const {
    return http_version_major_;
}