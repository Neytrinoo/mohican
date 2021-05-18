cmake_minimum_required(VERSION 3.16)

project(mohican)

find_package( Boost COMPONENTS log log_setup REQUIRED )

include_directories(${Boost_INCLUDE_DIR} config/include http/include server/include workers/include)

link_directories(${Boost_LIBRARY_DIR})

aux_source_directory(config/src CONFIG_SRC)
aux_source_directory(http/src HTTP_SRC)
aux_source_directory(server/src MOHICAN_SERVER_SRC)
aux_source_directory(workers/src WORKERS_SRC)

#add_executable(mohican.out config/${SRC} http/${SRC} server/${SRC} workers/${SRC})
add_executable(mohican.out ${CONFIG_SRC} ${HTTP_SRC} ${MOHICAN_SERVER_SRC} ${WORKERS_SRC})

target_link_libraries( mohican.out ${Boost_LIBRARIES} )
