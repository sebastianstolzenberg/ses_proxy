cmake_minimum_required(VERSION 3.0)

###############################################################################
option(HUNTER_ENABLED "Use hunter package manager for dependency resolution" OFF)

include("cmake/HunterGate.cmake")

set(HUNTER_STATUS_DEBUG OFF)
set(HUNTER_TLS_VERIFY ON)
HunterGate(
        URL "https://github.com/ruslo/hunter/archive/v0.20.39.tar.gz"
        SHA1 "b49c4b58e17c1473e822f8b21fcde92fee25791d"
)

###############################################################################
project(ses_proxy)

#add_subdirectory(cryptonote)

find_package(Threads)

hunter_add_package(OpenSSL)
find_package(OpenSSL REQUIRED)

SET(Boost_USE_STATIC_LIBS ON)
hunter_add_package(Boost COMPONENTS program_options log system)
find_package(Boost COMPONENTS program_options log log_setup system REQUIRED)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(src/cryptonote)

include_directories(src)

add_library(ses_proxy_util
        STATIC
        src/util/difficulty.cpp
        src/util/target.cpp
        src/util/log.cpp
        src/util/hashratecalculator.cpp
        src/util/pidcontroller.cpp)

add_library(ses_proxy_net
        STATIC
        src/net/connection.cpp
        src/net/client/connection.cpp
        src/net/client/boosttlsconnection.cpp
        src/net/client/boosttcpconnection.cpp
        src/net/server/server.cpp
        src/net/jsonrpc/jsonrpc.cpp src/net/client/http.cpp)

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
        src/proxy/blob.cpp
        src/proxy/workeridentifier.cpp
        src/proxy/configurationfile.cpp)

target_link_libraries(ses_proxy
        ses_proxy_net
        ses_proxy_stratum
        ses_proxy_util
        cryptonote
        Boost::program_options
        Boost::log_setup
        Boost::log
        Boost::system
        OpenSSL::SSL
        OpenSSL::Crypto
        ${CMAKE_THREAD_LIBS_INIT})
#add_dependencies(ses_proxy cryptonote)

add_subdirectory(test EXCLUDE_FROM_ALL)