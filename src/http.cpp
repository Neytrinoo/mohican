#include <fcntl.h>
#include <unistd.h>
#include <iostream>

#include "http_request.h"
#include "http_response.h"
#include "http_handle.h"

int main() {
    {
        int fd = open("get", O_RDONLY);
        HttpRequest http_request(fd);
        HttpResponse http_response = HttpHandler(http_request, ".");
        http_response.send(STDOUT_FILENO);
        close(fd);
    }
    std::cout << "\n\nANOTHER\n" << std::endl;
    {
        int fd = open("post", O_RDONLY);
        HttpRequest http_request(fd);
        HttpResponse http_response = HttpHandler(http_request, ".");
        http_response.send(STDOUT_FILENO);
        close(fd);
    }
    std::cout << "\n\nFILE\n" << std::endl;
    system("cat example.html");
    std::cout << "\n\nANOTHER\n" << std::endl;
    {
        int fd = open("delete", O_RDONLY);
        HttpRequest http_request(fd);
        HttpResponse http_response = HttpHandler(http_request, ".");
        http_response.send(STDOUT_FILENO);
        close(fd);
    }
    return 0;
}