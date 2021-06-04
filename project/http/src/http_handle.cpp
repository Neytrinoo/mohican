#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "http_date.h"
#include "http_exceptions.h"
#include "http_file_types.h"
#include "http_handle.h"

static std::string get_content_type(const std::string& url);

HttpResponse http_handler(const HttpRequest& request, const std::string& root) {
    if (request.get_major() != 1 || (request.get_minor() != 0 && request.get_minor() != 1)) {
        throw ProtVersionException("Wrong protocol version");
    }

    int status;
    std::string message;
    std::unordered_map<std::string, std::string> headers;
    headers[SERVER_HDR] = SERVER_VL;
    int file_fd;

    if (root == NO_ROOT) {
        status = NOT_FOUND_STATUS;
        message = NOT_FOUND_MSG;
    } else if (request.get_method() == GET_METHOD || request.get_method() == HEAD_METHOD) {
        if (request.get_url() == "/") {
            request.get_url() = DEFAULT_URL;
        }
        file_fd = open((root + request.get_url()).c_str(), O_RDONLY);
        struct stat file_stat;
        if (file_fd == NOT_OK || fstat(file_fd, &file_stat) == NOT_OK) {
            status = NOT_FOUND_STATUS;
            message = NOT_FOUND_MSG;
        } else {
            close(file_fd);
            try {
                std::string content_type = get_content_type(request.get_url());
                headers[CONTENT_TYPE_HDR] = content_type;
                headers[CONTENT_LENGTH_HDR] = std::to_string(file_stat.st_size);
                headers[CONNECTION_HDR] = CLOSE_VL;
                status = OK_STATUS;
                message = OK_MSG;
            } catch (WrongFileType&) {
                status = UNSUPPORTED_STATUS;
                message = UNSUPPORTED_MSG;
            }
        }
    } else {
        throw MethodException("Wrong method");
    }

    headers[DATE_HDR] = Date::get_date();
    headers[CONNECTION_HDR] = CLOSE_VL;
    return HttpResponse(headers, request.get_major(), request.get_minor(), status, message);
}

static std::string get_content_type(const std::string& url) {
    size_t ext_pos = url.rfind('.');
    size_t slash_pos = url.rfind('/');
    if (ext_pos < slash_pos && slash_pos != std::string::npos) {
        ext_pos = std::string::npos;
    }
    
    std::string content_type;
    if (ext_pos != std::string::npos) {
        ext_pos++;
        std::string ext = url.substr(ext_pos, url.size() - ext_pos);
        auto iter = types.find(ext);
        if (iter == types.end())
            throw WrongFileType("Unsupported file type");
        content_type = iter->second;
    } else {
        content_type = types.find("txt")->second;
    }
    return content_type;
}
