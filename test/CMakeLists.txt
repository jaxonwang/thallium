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


file(GLOB unit_test_src 
    "unit_test/*.cpp"
    )
add_executable(unit_test ${unit_test_src})


file(GLOB i_test_src 
    "integration_test/*.cpp"
    )
add_executable(integration_test ${i_test_src})

if(APPLE)
    add_definitions(-DBOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED)
    elseif(UNIX AND NOT APPLE)
    target_link_libraries(integration_test dl)
endif()

include("${catch_SOURCE_DIR}/contrib/Catch.cmake")
catch_discover_tests(unit_test)
