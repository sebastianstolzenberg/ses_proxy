cmake_minimum_required(VERSION 3.0)
project(cryptonight)

set(CMAKE_CXX_STANDARD 14)

add_library(cryptonight
        STATIC
        c_blake256.c
        c_groestl.c
        c_jh.c
        c_keccak.c
        c_skein.c
        CryptoNight.cpp)
target_include_directories(cryptonight PUBLIC . PRIVATE ..)
set_target_properties(cryptonight PROPERTIES CXX_FLAGS "-fexceptions -frtti")
