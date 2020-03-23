cmake_minimum_required(VERSION 3.10)

# set the project name
project(thallium VERSION 0.1)

# add the executable
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THALLIUM_PROJECT_DIRECTORY, ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-long-long -pedantic")

find_package(Boost REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories("include")

# testing
enable_testing ()
option (THALLIUM_BUILD_TESTS "Build test programs" ON)
if (THALLIUM_BUILD_TESTS)
    add_subdirectory(test)
endif ()

# examples
