#pragma once

#include <exception>
#include <string>

class BaseConfigFileException : public std::exception {
public:
    BaseConfigFileException(const std::string &msg) : msg_(std::move(msg)) {}

    const char *what() const noexcept override {
        return msg_.c_str();
    }

protected:
    std::string msg_;
};

class NoSuchConfigFileException : public BaseConfigFileException {
public:
    NoSuchConfigFileException(const std::string &msg) : BaseConfigFileException(msg) {}
};

class InvalidConfigException : public BaseConfigFileException {
public:
    InvalidConfigException(const std::string &msg) : BaseConfigFileException(msg) {}
};