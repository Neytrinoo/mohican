#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "defines.h"
#include "http_handle.h"
#include "http_exceptions.h"

HttpResponse HttpHandler(const HttpRequest &request, const std::string &root) {
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

    if (request.get_method() == "GET" || request.get_method() == "HEAD") {
        method = GET;
        if (request.get_url() == "/") {
            request.get_url() = "/index.html";
        }
        file_fd = open((root + request.get_url()).c_str(), O_RDONLY);
        struct stat file_stat;
        if (file_fd == NOTOK || fstat(file_fd, &file_stat) == NOTOK) {
            status = 404;
            message = "not found";
            header = "connection";
            value = "close";
            headers[header] = value;
        } else {
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
            status = 200;
            message = "OK";
            if (request.get_method() == "HEAD") {
                close(file_fd);
            }
        }
        if (request.get_method() == "HEAD") {
            file_fd = NOFILE;
            method = HEAD;
        }
    } else if (request.get_method() == "DELETE") {
        method = DELETE;
        if (request.get_url() == "/") {
            request.get_url() = "/index.html";
        }
        file_fd = open((root + request.get_url()).c_str(), O_RDONLY);
        if (file_fd == NOTOK) {
            status = 204;
            message = "no content";
            header = "connection";
            value = "close";
            headers[header] = value;
        } else {
            close(file_fd);
            remove((root + request.get_url()).c_str());
            status = 200;
            message = "OK";
        }
        file_fd = NOFILE;
    } else if (request.get_method() == "PUT" || request.get_method() == "POST") {
        if (request.get_method() == "PUT") {
            method = PUT;
        } else {
            method = POST;
        }
        std::string filename = root + request.get_url();
        if (access(filename.c_str(), F_OK) == OK) {
            status = 200;
            message = "OK";
        } else {
            status = 201;
            message = "Created";
        }
        header = "content-location";
        value = request.get_url();
        headers[header] = value;
        file_fd = open(filename.c_str(), O_WRONLY);
        char buffer[BUFSIZE];
        while (true) {
            int r = read(request.get_body(), buffer, sizeof(buffer));
            if (r < 0)
                throw ReadException("Cannot read body");
            if (r == 0)
                break;
            if (write(file_fd, buffer, r) == NOTOK)
                throw WriteException("Cannot write file");
        }
        close(file_fd);
        file_fd = NOFILE;
    }

    HttpResponse response(headers, request.get_major(), request.get_minor(),
                          (root + request.get_url()), status, message, file_fd, method);

    return response;
}
