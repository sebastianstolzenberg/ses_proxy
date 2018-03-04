cmake_minimum_required(VERSION 3.9)

include(ExternalProject)

ExternalProject_Add(
        cryptonote

        GIT_REPOSITORY "https://github.com/lucasjones/node-cryptonote-util.git"
        GIT_TAG "master"

        UPDATE_COMMAND "pwd"
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy  ${CMAKE_CURRENT_SOURCE_DIR}/cryptonote_CMakeLists.txt CMakeLists.txt
)

ExternalProject_Get_Property(cryptonote INSTALL_DIR)
ExternalProject_Get_Property(cryptonote BINARY_DIR)

set(cryptonote_INCLUDE_DIR ${INSTALL_DIR})
set(cryptonote_BINARY ${BINARY_DIR}"/cryptonote.a")