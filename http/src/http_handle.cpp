#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "http_exceptions.h"
#include "http_file_types.h"
#include "http_handle.h"

static std::string get_content_type(size_t ext_pos, const std::string& url);

HttpResponse http_handler(const HttpRequest &request, const std::string &root) {
    if (request.get_major() != 1 || (request.get_minor() != 0 && request.get_minor() != 1)) {
        throw ProtVersionException("Wrong protocol version");
    }

    int status = 0;
    std::string message;
    std::string header = "server";
    std::string value = "mohican";
    std::unordered_map<std::string, std::string> headers;
    headers[header] = value;
    int file_fd;

    if (root == NO_ROOT) {
        status = NOT_FOUND_STATUS;
        message = NOT_FOUND_MSG;
    } else if (request.get_method() == "GET" || request.get_method() == "HEAD") {
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
            size_t ext_pos = request.get_url().rfind('.');
            try {
                std::string content_type = get_content_type(ext_pos, request.get_url());
                headers[CONTENT_TYPE_HDR] = content_type;
                headers[CONTENT_LENGTH_HDR] = std::to_string(file_stat.st_size);
                status = OK_STATUS;
                message = OK_MSG;
            } catch (WriteException &e) {
                status = UNSUPPORTED_STATUS;
                message = UNSUPPORTED_MSG;
            }
        }
    } else {
        throw MethodException("Wrong method");
    }

    headers[CONNECTION_HDR] = CLOSE_VL;
    return HttpResponse(headers, request.get_major(), request.get_minor(), status, message);
}

static std::string get_content_type(size_t ext_pos, const std::string& url) {
    std::string content_type;
    if (ext_pos != std::string::npos) {
        ext_pos++;
        std::string ext = url.substr(ext_pos, url.size() - ext_pos);
        if (ext == HTML_EXT) {
            content_type = HTML_TYPE;
        } else if (ext == JPG_EXT) {
            content_type = JPG_TYPE;
        } else if (ext == GIF_EXT) {
            content_type = GIF_TYPE;
        } else {
            throw WrongFileType("Unsupported file type");
        }
    } else {
        content_type = PLAIN_TYPE;
    }
    return content_type;
}
