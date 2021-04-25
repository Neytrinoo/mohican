#include "http_base.h"

HttpBase::HttpBase() : http_version_major_(1), http_version_minor_(0) {}

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

