#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>

#include "http_request.h"
#include "http_response.h"
#include "http_handle.h"

int main() {
    {
        int fd = open("get", O_RDONLY);
        struct stat stat;
        fstat(fd, &stat);
        char *file_c = static_cast<char *>(mmap(NULL, stat.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0));
        std::string file(file_c);
        HttpRequest http_request(file);
        HttpResponse http_response = http_handler(http_request, ".");
        close(fd);
        std::cout << http_response.get_string();
    }
    return 0;
}