#include "http_base.h"

HttpBase::HttpBase() : version_major_(1), version_minor_(0) {}

HttpBase::HttpBase(std::vector<HttpHeader> &headers, int major, int minor)
        : request_headers_(headers), version_major_(major), version_minor_(minor) {}

void HttpBase::set_version(int major, int minor) {
    version_major_ = major;
    version_minor_ = minor;
}

void HttpBase::set_headers(std::vector<HttpHeader> headers) {
    request_headers_ = std::move(headers);
}

int &HttpBase::get_minor() {
    return version_minor_;
}

int HttpBase::get_minor() const {
    return version_minor_;
}

int &HttpBase::get_major() {
    return version_major_;
}

int HttpBase::get_major() const {
    return version_major_;
}

