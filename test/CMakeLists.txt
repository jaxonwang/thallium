cmake_minimum_required(VERSION 3.10)

# add_library(Catch2 INTERFACE)
# ADD_LIBRARY(Catch2::Catch2 ALIAS Catch2)
# target_include_directories(Catch2 INTERFACE catch.hpp)

add_executable(common_header_test main_test.cpp common.cpp)
# target_link_libraries(common_header_test Catch2)


add_test(common_header_test common_header_test)
