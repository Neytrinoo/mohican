cmake_minimum_required(VERSION 3.16)

project(mohican)

include_directories(include)

add_executable(main src/main.cpp src/main_server_settings.cpp src/parse_functions.cpp src/server_settings.cpp)