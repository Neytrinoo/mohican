cmake_minimum_required(VERSION 3.16)

project(mohican)

include_directories(config/include http/include mohican_server/include workers/include)

aux_source_directory(config/src CONFIG_SRC)
aux_source_directory(http/src HTTP_SRC)
aux_source_directory(mohican_server/src MOHICAN_SERVER_SRC)
aux_source_directory(workers/src WORKERS_SRC)

#add_executable(mohican.out config/${SRC} http/${SRC} mohican_server/${SRC} workers/${SRC})
add_executable(mohican.out ${CONFIG_SRC} ${HTTP_SRC} ${MOHICAN_SERVER_SRC} ${WORKERS_SRC})