cmake_minimum_required(VERSION 3.0)


option(HUNTER_ENABLED "Use hunter package manager for dependency resolution" OFF)
option(CC_SUPPORT "Enable support for XMRIGCC's Control Center" OFF)

###############################################################################
include("cmake/HunterGate.cmake")

set(HUNTER_STATUS_DEBUG OFF)
set(HUNTER_TLS_VERIFY ON)

HunterGate(
        URL "https://github.com/ruslo/hunter/archive/v0.22.19.tar.gz"
        SHA1 "54749ffc945f78362aa680a7abbafcaa40ff9069"
)

###############################################################################

project(ses_proxy)

find_package(Threads)

hunter_add_package(OpenSSL)
find_package(OpenSSL REQUIRED)

if (HUNTER_ENABLED)
    set(Boost_NO_SYSTEM_PATHS ON)
endif()
set(Boost_USE_STATIC_LIBS ON)
hunter_add_package(Boost COMPONENTS program_options log thread system)
if (CC_SUPPORT)
    find_package(Boost 1.66 COMPONENTS program_options log log_setup thread system REQUIRED)
else()
    find_package(Boost COMPONENTS program_options log log_setup thread system REQUIRED)
endif()

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(src/cryptonote)

include_directories(src ${Boost_INCLUDE_DIR})

add_library(ses_proxy_util
        STATIC
        src/util/difficulty.cpp
        src/util/target.cpp
        src/util/log.cpp
        src/util/hashratecalculator.cpp
        src/util/pidcontroller.cpp
        src/util/shell.cpp
        )

add_library(ses_proxy_net
        STATIC
        src/net/connection.cpp
        src/net/client/connection.cpp
        src/net/client/boosttlsconnection.cpp
        src/net/client/boosttcpconnection.cpp
        src/net/server/server.cpp
        src/net/jsonrpc/jsonrpc.cpp
        $<$<STREQUAL:${CC_SUPPORT},ON>:src/net/client/http.cpp>
        )

add_library(ses_proxy_stratum
        STATIC
        src/stratum/stratum.cpp
        src/stratum/job.cpp)

add_executable(ses_proxy
        src/main.cpp
        src/proxy/server.cpp
        src/proxy/client.cpp
        src/proxy/clientstatistics.cpp
        src/proxy/pool.cpp
        src/proxy/proxy.cpp
        src/proxy/jobresult.cpp
        src/proxy/job.cpp
        src/proxy/jobtemplate.cpp
        src/proxy/worker.cpp
        src/proxy/blob.cpp
        src/proxy/workeridentifier.cpp
        src/proxy/configurationfile.cpp
        $<$<STREQUAL:${CC_SUPPORT},ON>:src/proxy/ccclient.cpp>
        src/proxy/algorithm.cpp)

if (CC_SUPPORT)
    target_compile_definitions(ses_proxy PRIVATE "-DCC_SUPPORT")
endif()

target_link_libraries(ses_proxy
        ses_proxy_net
        ses_proxy_stratum
        ses_proxy_util
        cryptonote
        ${Boost_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${CMAKE_DL_LIBS})
#add_dependencies(ses_proxy cryptonote)

add_subdirectory(test EXCLUDE_FROM_ALL)