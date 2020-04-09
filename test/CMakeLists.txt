cmake_minimum_required(VERSION 3.11)

# add_library(Catch2 INTERFACE)
# ADD_LIBRARY(Catch2::Catch2 ALIAS Catch2)
# target_include_directories(Catch2 INTERFACE catch.hpp)

# test framwork
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

file(GLOB unit_test_src 
    "unit_test/*.cpp"
    )
add_executable(unit_test ${unit_test_src})
target_link_libraries(example gtest_main)


file(GLOB i_test_src 
    "integration_test/*.cpp"
    )
add_executable(integration_test ${i_test_src})

# using boost stactrace when debug
if(APPLE)
    add_definitions(-DBOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED)
    elseif(UNIX AND NOT APPLE)
    add_definitions(-DBOOST_STACKTRACE_USE_BACKTRACE)
    target_link_libraries(integration_test dl)
    target_link_libraries(integration_test backtrace)
endif()
