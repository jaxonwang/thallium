cmake_minimum_required(VERSION 3.11)

# set the project name
project(thallium VERSION 0.1)

# add the executable
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Debug CACHE STRING "Type of build")
SET(ISDEBUG (${CMAKE_BUILD_TYPE} MATCHES Debug))

# enable lto
include(CheckIPOSupported)
cmake_policy(SET CMP0069 NEW)
check_ipo_supported(RESULT ipo_enabled OUTPUT output)

# enable warning
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-long-long -pedantic")

# enable santinizer
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined")

include(FetchContent)

function(populate_unless_exist name)
    FetchContent_GetProperties(${name})
    if(NOT ${name}_POPULATED)
        message("Fetching ${name} ...")
        FetchContent_Populate(${name})
        message("Done.")
    endif()
    set(${name}_SOURCE_DIR ${${name}_SOURCE_DIR} PARENT_SCOPE)
endfunction(populate_unless_exist)

# boost // boost why are you so large?
find_package(Boost REQUIRED)
include_directories("${Boost_INCLUDE_DIR}")

set(THALLIUM_PROJECT_DIRECTORY, ${CMAKE_CURRENT_SOURCE_DIR})

# for this project

file(GLOB src_files
    "src/*.cpp"
    )
add_library(thallium STATIC ${src_files})
add_library(thallium_shared SHARED ${src_files})
if( NOT ${ISDEBUG} AND ipo_enabled )
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET thallium_shared PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
add_library(thallium_header INTERFACE)
target_compile_definitions(thallium_header
    INTERFACE LIBRARY_HEADER_ONLY
)

include_directories("include")


# testing
enable_testing ()
option (THALLIUM_BUILD_TESTS "Build test programs" ON)
if (THALLIUM_BUILD_TESTS)
    add_subdirectory(test)
endif ()

# examples

