#include <exception>
#include <string>

class BaseException : public std::exception {
 public:
    BaseException(const std::string &msg) : msg_(std::move(msg)) {}

    const char *what() const noexcept override {
        return msg_.c_str();
    }

 private:
    std::string msg_;
};

class ReadException : public  BaseException {
 public:
    ReadException(const std::string &msg) : BaseException(msg) {}
};

class DelimException : public BaseException {
 public:
    DelimException(const std::string &msg) : BaseException(msg) {}
};

class ProtVersionException : public BaseException {
 public:
    ProtVersionException(const std::string &msg) : BaseException(msg) {}
};