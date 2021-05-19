#include "http_response.h"

HttpResponse::HttpResponse(std::unordered_map<std::string, std::string> headers,
                           int major,
                           int minor,
                           int status,
                           const std::string &message)
        : HttpBase(headers, major, minor),
          status_(status),
          message_(message) {}

std::string HttpResponse::get_string() {
    std::string str;
    str += "HTTP/" + std::to_string(get_major()) + "." +
            std::to_string(get_minor()) + " " + std::to_string(status_) + " " + message_ + "\r\n";
    for (const auto &header: headers_) {
        str += header.first + ": " + header.second + "\r\n";
    }
    str += "\n";
    return str;
}
