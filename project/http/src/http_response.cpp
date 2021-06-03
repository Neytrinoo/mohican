#include "http_exceptions.h"
#include "http_response.h"
#include "string_to_lower.h"

HttpResponse::HttpResponse(std::unordered_map<std::string, std::string> headers,
                           int major,
                           int minor,
                           int status,
                           const std::string& message)
        : HttpBase(headers, major, minor),
          status_(status),
          message_(message) {}

std::string HttpResponse::get_string() {
    std::string str;
    str += "HTTP/" + std::to_string(get_major()) + "." +
            std::to_string(get_minor()) + " " + std::to_string(status_) + " " + message_ + "\r\n";
    for (const auto& header: headers_) {
        str += header.first + ": " + header.second + "\r\n";
    }
    str += "\r\n";
    return str;
}
int HttpResponse::get_status() const {
    return status_;
}
void HttpResponse::add_line(const std::string& line) {
    if (line.length() == 2 && line[0] == '\r') {
        response_ended_ = true;
        return;
    }
    if (!first_line_added_) {
        add_first_line(line);
        return;
    }
    if (!headers_read_) {
        add_header(line);
        return;
    }
}

void HttpResponse::add_first_line(const std::string& line) {
    size_t lf_pos = line.find('\n');
    if (lf_pos == std::string::npos) {
        throw DelimException("Line feed not found");
    }
    if (sscanf(line.c_str(), "HTTP/%d.%d %d", &version_major_, &version_minor_, &status_) != 3) {
        throw ReadException("Error while reading from file descriptor");
    }
    size_t start_pos = line.rfind(' ');
    if (start_pos == std::string::npos) {
        throw DelimException("Space not found");
    }
    message_ = std::string(line, start_pos + 1, lf_pos - 1 - start_pos);
    first_line_added_ = true;
}

void HttpResponse::add_header(const std::string& line) {
    size_t start_pos = 0;
    size_t lf_pos;
    lf_pos = line.find('\n', start_pos);
    if (lf_pos == std::string::npos) {
        throw DelimException("Line feed not found");
    }
    if (lf_pos == start_pos || line[start_pos] == '\r') {
        headers_read_ = true;
        return;
    }
    size_t colon_pos = line.find(':', start_pos);
    std::string header_name(line, start_pos, colon_pos - start_pos);
    if (colon_pos == std::string::npos) {
        throw DelimException("Colon not found");
    }

    start_pos = colon_pos + 1;
    while (line[start_pos] == ' ') {
        start_pos++;
    }
    std::string header_value(line, start_pos, lf_pos - 1 - start_pos);
    string_to_lower(header_name);
    headers_[header_name] = header_value;
}

bool HttpResponse::response_ended() const {
    return response_ended_;
}
