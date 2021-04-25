#include "http_parse.h"

int main() {
    std::vector<HttpHeader> headers;
    headers.resize(1);
    std::string str1 = "ahahah";
    std::string str2 = "ekekke";
    headers[0].setHeader(str1, str2);
    std::string str3 = "i love math";
    std::string str4 = "ok kniga";
    HttpRequest http_request(str3, str4, headers, 1, 0);
    return 0;
}