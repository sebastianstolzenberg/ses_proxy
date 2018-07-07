cmake_minimum_required(VERSION 3.0)
project(cryptonote)

set(CMAKE_CXX_STANDARD 14)

add_library(cryptonote
        STATIC
        crypto/hash.c
        crypto/keccak.c
        crypto/tree-hash.c
        cryptonote_core/cryptonote_format_utils.cpp
        cryptonote.cpp)
target_include_directories(cryptonote PUBLIC . ${Boost_INCLUDE_DIR} PRIVATE ..)
set_target_properties(cryptonote PROPERTIES CXX_FLAGS "-fexceptions -frtti")
