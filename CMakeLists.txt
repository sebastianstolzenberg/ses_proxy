cmake_minimum_required(VERSION 3.9)
project(ses_proxy)

#add_subdirectory(cryptonote)

add_subdirectory(src/cryptonote)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED system)

include_directories(src)

add_library(ses_proxy_net
        STATIC
        src/net/connection.cpp
        src/net/client/connection.cpp
        src/net/client/boosttlsconnection.cpp
        src/net/client/boosttcpconnection.cpp
        src/net/server/server.cpp
        src/net/server/server.hpp
        src/net/jsonrpc/jsonrpc.cpp)

add_library(ses_proxy_stratum
        STATIC
        src/stratum/stratum.cpp
        src/stratum/job.cpp)

add_executable(ses_proxy
        src/main.cpp
        src/proxy/server.cpp
        src/proxy/client.cpp
        src/proxy/pool.cpp
        src/proxy/proxy.cpp
        src/proxy/jobresult.cpp
        src/proxy/job.cpp
        src/proxy/jobtemplate.cpp
        src/proxy/worker.cpp
        src/proxy/blob.cpp src/proxy/workeridentifier.cpp)
target_link_libraries(ses_proxy
        ses_proxy_net
        ses_proxy_stratum
        cryptonote
        Boost::system
        OpenSSL::SSL
        OpenSSL::Crypto
        ${CMAKE_THREAD_LIBS_INIT})
#add_dependencies(ses_proxy cryptonote)