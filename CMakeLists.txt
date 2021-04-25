cmake_minimum_required(VERSION 3.16)
project(nginx_the_best_copy)

include_directories(include)

add_executable(http_test src/http_test.cpp
                         src/http_parse.cpp)
