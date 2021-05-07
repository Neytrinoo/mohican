#include <sys/stat.h>
#include <fcntl.h>

#include "http_request.h"
#include "http_response.h"
#include "http_handle.h"

int main() {
    int fd = open("request", O_RDONLY);
    HttpRequest http_request(fd);
    return 0;
}