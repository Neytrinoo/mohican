#pragma once

#include <exception>
#include <string>

class BaseServerSettingsException : public std::exception {
 public:
    BaseServerSettingsException(const std::string &msg) : msg_(std::move(msg)) {}

    const char *what() const noexcept override {
        return msg_.c_str();
    }

 private:
    std::string msg_;
};

class RootNotFoundException : public  BaseServerSettingsException {
 public:
    RootNotFoundException(const std::string &msg) : BaseServerSettingsException(msg) {}
};