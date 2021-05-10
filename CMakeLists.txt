cmake_minimum_required(VERSION 3.16)

project(mohican)

include_directories(include)

aux_source_directory(src SRC)

add_executable(mohican.out ${SRC})