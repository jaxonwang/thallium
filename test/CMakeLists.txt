cmake_minimum_required(VERSION 3.11)

# add_library(Catch2 INTERFACE)
# ADD_LIBRARY(Catch2::Catch2 ALIAS Catch2)
# target_include_directories(Catch2 INTERFACE catch.hpp)

# test framwork
FetchContent_Declare(
    catch
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    GIT_TAG v2.11.3
    )

populate_unless_exist(catch)
include_directories("${catch_SOURCE_DIR}/single_include")


file(GLOB test_src 
    "*.cpp"
)
add_executable(unit_test ${test_src})

include("${catch_SOURCE_DIR}/contrib/Catch.cmake")
catch_discover_tests(unit_test)
