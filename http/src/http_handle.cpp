#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "http_exceptions.h"
#include "http_file_types.h"
#include "http_handle.h"

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
        header = CONNECTION_HDR;
        value = CLOSE_VL;
        headers[header] = value;
    } else if (request.get_method() == "GET" || request.get_method() == "HEAD") {
        if (request.get_url() == "/") {
            request.get_url() = DEFAULT_URL;
        }
        file_fd = open((root + request.get_url()).c_str(), O_RDONLY);
        struct stat file_stat;
        if (file_fd == NOT_OK || fstat(file_fd, &file_stat) == NOT_OK) {
            status = NOT_FOUND_STATUS;
            message = NOT_FOUND_MSG;
            header = CONNECTION_HDR;
            value = CLOSE_VL;
            headers[header] = value;
        } else {
            std::string content_type;
            size_t ext_pos = request.get_url().rfind('.');
            if (ext_pos != std::string::npos) {
                ext_pos++;
                std::string ext = request.get_url().substr(ext_pos, request.get_url().size() - ext_pos);
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
            headers[CONTENT_TYPE_HDR] = content_type;
            headers[CONTENT_LENGTH_HDR] = std::to_string(file_stat.st_size);
            status = OK_STATUS;
            message = OK_MSG;
            close(file_fd);
        }
    } else {
        throw MethodException("Wrong method");
    }

    return HttpResponse(headers, request.get_major(), request.get_minor(), status, message);
}
