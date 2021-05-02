cmake_minimum_required(VERSION 3.16)
project(mohican)

include_directories(include)

add_executable(http src/http.cpp
        src/http_request.cpp
        src/http_response.cpp
        src/http_header.cpp
        src/http_base.cpp)
