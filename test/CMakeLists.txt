cmake_minimum_required(VERSION 3.11)

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
# enabling gtest dirs
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

# add include dir in src
include_directories(${CMAKE_SOURCE_DIR}/src .)

file(GLOB test_util_src "*.cpp")
add_library(test_util STATIC ${test_util_src})

file(GLOB unit_test_src 
    "unit_test/*.cpp"
    )
add_executable(unit_test ${unit_test_src})
target_link_libraries(unit_test gtest_main thallium test_util)

include(GoogleTest)
gtest_add_tests(TARGET unit_test)

# using boost stactrace when debug
function(addbacktrace TARGETNAME)
if(APPLE)
    add_definitions(-DBOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED)
    elseif(UNIX AND NOT APPLE)
    add_definitions(-BOOST_STACKTRACE_USE_ADDR2LINE)
    target_link_libraries(${TARGETNAME} dl)
endif()
endfunction(addbacktrace)

# for each cpp in integration test folder create one binary
file(GLOB i_test_src 
    "integration_test/*.cpp"
    )
foreach(cppfile ${i_test_src})
    get_filename_component(target_name ${cppfile} NAME_WE)
    add_executable(${target_name} ${cppfile})
    target_link_libraries(${target_name} thallium)
    addbacktrace(${target_name})

endforeach()
