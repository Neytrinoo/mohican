#include <fcntl.h>
#include <cstring>
#include <sys/stat.h>

#include "handler_defines.h"
#include "http_handle.h"
#include "http_exceptions.h"

HttpResponse HttpHandler(const std::string &root, int sock) {
    HttpRequest request(sock);

    if (request.get_major() != 1 || (request.get_minor() != 0 && request.get_minor() != 1)) {
        throw ProtVersionException("Wrong protocol version");
    }

    int status = 0;
    std::string message;
    std::string header = "server";
    std::string value = "mohican";
    std::unordered_map<std::string, std::string> headers;
    headers[header] = value;
    int file_fd = NOFILE;
    int method = 0;

    if (request.get_method() == "GET") {
        method = GET;
        if (request.get_url() == "/") {
            request.get_url() = "/index.html";
        }
        file_fd = open((root + request.get_url()).c_str(), O_RDONLY);
        struct stat file_stat;
        if (file_fd == -1 || fstat(file_fd, &file_stat) == -1) {
            status = 404;
            message = "not found";
            header = "connection";
            value = "close";
            headers[header] = value;
        }

        std::string content_type;
        const char *ext = strrchr((root + request.get_url()).c_str(), '.');
        if (ext) {
            ext++;
            if (strcmp(ext, "html") == 0)
                content_type = "text/html";
            else if (strcmp(ext, "jpg") == 0)
                content_type = "image/jpg";
            else if (strcmp(ext, "gif") == 0)
                content_type = "image/gif";
        }
        headers["content-type"] = content_type;
        headers["content-length"] = std::to_string(file_stat.st_size);
    }

    HttpResponse response(headers, request.get_major(), request.get_minor(),
                          (root + request.get_url()), status, message, file_fd, method);

    return response;
}
