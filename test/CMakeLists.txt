cmake_minimum_required(VERSION 3.0)

include_directories(../src)

add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE catch)

add_executable(test_util util/test_util.cpp)
target_link_libraries(test_util ses_proxy_util Catch Boost::log)

enable_testing()
add_test(NAME test_util COMMAND test_util)