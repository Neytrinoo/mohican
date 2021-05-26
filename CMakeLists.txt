cmake_minimum_required(VERSION 3.16)

project(mohican)

find_package( Boost COMPONENTS log log_setup REQUIRED )

include_directories(${Boost_INCLUDE_DIR} project/config/include project/http/include project/server/include project/workers/include project/logs/include)

link_directories(${Boost_LIBRARY_DIR})

aux_source_directory(project/config/src CONFIG_SRC)
aux_source_directory(project/http/src HTTP_SRC)
aux_source_directory(project/server/src MOHICAN_SERVER_SRC)
aux_source_directory(project/workers/src WORKERS_SRC)
aux_source_directory(project/logs/src LOGS_SRC)

#add_executable(mohican.out config/${SRC} http/${SRC} server/${SRC} workers/${SRC})
add_executable(mohican.out ${CONFIG_SRC} ${HTTP_SRC} ${MOHICAN_SERVER_SRC} ${WORKERS_SRC} ${LOGS_SRC})

target_link_libraries(mohican.out ${Boost_LIBRARIES} )
