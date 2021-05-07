#pragma once

#include "http_request.h"
#include "http_response.h"

HttpResponse HttpHandler(const std::string &root, int sock);
