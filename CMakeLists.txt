cmake_minimum_required(VERSION 3.10.0)
project(web_serv VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)

add_subdirectory(netlib)

add_executable(web_serv main.cpp
)

target_link_libraries(web_serv PRIVATE netlib)

