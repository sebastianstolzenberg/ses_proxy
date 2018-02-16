cmake_minimum_required(VERSION 3.9)
project(ses_proxy)

set(CMAKE_CXX_STANDARD 11)

find_package(Boost)

include_directories(src)

add_library(ses_proxy_net STATIC src/net/client/connection.cpp src/net/client/boosttlsconnection.cpp src/net/client/boosttcpconnection.cpp src/net/server/server.cpp src/net/server/server.hpp src/net/server/connection.cpp src/net/server/connection.hpp)

add_executable(ses_proxy src/main.cpp)
target_link_libraries(ses_proxy ses_proxy_net)