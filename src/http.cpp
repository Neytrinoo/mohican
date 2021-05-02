#include "http_parse.h"
#include <sys/stat.h>
#include <fcntl.h>
int main() {
    int fd = open("request", O_RDONLY);
    HttpRequest http_request(fd);
    return 0;
}