cmake_minimum_required(VERSION 3.9)
project(cryptonote)

set(CMAKE_CXX_STANDARD 17)

add_library(cryptonote
        STATIC
        crypto/hash.c
        crypto/keccak.c
        crypto/tree-hash.c
        cryptonote_core/cryptonote_format_utils.cpp
        cryptonote.cpp cryptonote.hpp)
target_include_directories(cryptonote PUBLIC .)
set_target_properties(cryptonote PROPERTIES CXX_FLAGS "-fexceptions -frtti -fpermissive")
