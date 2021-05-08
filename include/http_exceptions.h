#include <exception>
#include <string>

class HttpBaseException : public std::exception {
 public:
    HttpBaseException(const std::string &msg) : msg_(std::move(msg)) {}

    const char *what() const noexcept override {
        return msg_.c_str();
    }

 private:
    std::string msg_;
};

class ReadException : public  HttpBaseException {
 public:
    ReadException(const std::string &msg) : HttpBaseException(msg) {}
};

class DelimException : public HttpBaseException {
 public:
    DelimException(const std::string &msg) : HttpBaseException(msg) {}
};

class ProtVersionException : public HttpBaseException {
 public:
    ProtVersionException(const std::string &msg) : HttpBaseException(msg) {}
};

class WriteException : public HttpBaseException {
 public:
    WriteException(const std::string &msg) : HttpBaseException(msg) {}
};
