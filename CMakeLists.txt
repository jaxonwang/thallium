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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra -Wno-long-long -pedantic")

# enable santinizer
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined")

# log level
if(${CMAKE_BUILD_TYPE} MATCHES Debug)
    add_definitions(-DLOG_LEVEL_DEBUG)
else()
    add_definitions(-DLOG_LEVEL_INFO)
endif()

# boost // boost why are you so large?
set(Boost_USE_MULTITHREADED FALSE)
find_package(Boost 1.67 REQUIRED COMPONENTS system)
include_directories("${Boost_INCLUDE_DIR}")

set(THALLIUM_PROJECT_DIRECTORY, ${CMAKE_CURRENT_SOURCE_DIR})

# use pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


# for this project
include_directories(include src)
set(libraries_thallium_deps ${Boost_LIBRARIES} Threads::Threads)

# find all cpp files
file(GLOB src_files
    "src/*.cpp"
    "src/network/*.cpp"
    )

# static lib
add_library(thallium STATIC ${src_files})
target_link_libraries(thallium PRIVATE ${libraries_thallium_deps})

# shared lib
add_library(thallium_shared SHARED ${src_files})
target_link_libraries(thallium_shared PRIVATE ${libraries_thallium_deps})
if(${CMAKE_BUILD_TYPE} MATCHES Release AND ipo_enabled)
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET thallium_shared PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# header lib
add_library(thallium_header INTERFACE)
target_link_libraries(thallium_header INTERFACE ${libraries_thallium_deps})
target_compile_definitions(thallium_header
    INTERFACE LIBRARY_HEADER_ONLY
)

# testing
enable_testing ()
option (THALLIUM_BUILD_TESTS "Build test programs" ON)
if (THALLIUM_BUILD_TESTS)
    add_subdirectory(test)
endif ()

# examples
