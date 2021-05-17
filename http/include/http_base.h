#pragma once

#include <string>
#include <unordered_map>

class HttpBase {
 public:
    HttpBase() = default;
    HttpBase(const std::unordered_map<std::string, std::string> &headers, int major = -1, int minor = -1);
    explicit HttpBase(const HttpBase &other) = default;
    HttpBase &operator=(const HttpBase &other) = default;
    ~HttpBase() = default;

    int get_minor() const;
    int get_major() const;
    std::unordered_map<std::string, std::string> &get_headers();

 protected:
    int version_major_;
    int version_minor_;
    std::unordered_map<std::string, std::string> headers_;
};
