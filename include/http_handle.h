#pragma once

#include "http_request.h"
#include "http_response.h"

HttpResponse HttpHandler(const HttpRequest &request, const std::string &root);
