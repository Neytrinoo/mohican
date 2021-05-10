#include <unistd.h>

#include "http_response.h"

HttpResponse::HttpResponse(std::unordered_map<std::string, std::string> headers,
                           int major,
                           int minor,
                           const std::string &filename,
                           int status,
                           const std::string &message)
        : HttpBase(headers, major, minor),
          filename_(filename),
          status_(status),
          message_(message) {}

std::string HttpResponse::get_string() {
    std::string str;
    str += "HTTP/" + std::to_string(get_major()) + "." +
            std::to_string(get_minor()) + " " + std::to_string(status_) + " " + message_ + "\n";
    for (const auto &header: headers_) {
        str += header.first + ": " + header.second + "\n";
    }
    str += "\n";
    return str;
}
