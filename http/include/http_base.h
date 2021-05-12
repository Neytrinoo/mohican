#pragma once

#include <string>
#include <unordered_map>

class HttpBase {
 public:
    HttpBase() = default;
    HttpBase(const std::unordered_map<std::string, std::string> &headers, int major = 0, int minor = 0);
    explicit HttpBase(const HttpBase &other) = default;
    HttpBase &operator=(const HttpBase &other) = default;
    ~HttpBase() = default;

    int get_minor() const;
    int get_major() const;
    std::unordered_map<std::string, std::string> &get_headers();

 protected:
    int read_line(const int fd, char *buffer);

 protected:
    int version_major_ = -1;
    int version_minor_ = -1;
    std::unordered_map<std::string, std::string> headers_;
    static const int buf_size_ = 4096;
    static const int read_size_ = 256;
};
