#pragma once

#include <string>
#include <vector>

#include "http_base.h"
#include "http_request.h"

class HttpResponse : public HttpBase {
 public:
    HttpResponse() = default;
    HttpResponse(const int fd, const HttpRequest &request);

 private:
    std::string body_;
};