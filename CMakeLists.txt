cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(watchdog)

# Find Tarantool and Lua dependecies
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set(TARANTOOL_FIND_REQUIRED ON)
find_package(Tarantool REQUIRED)
include_directories(${TARANTOOL_INCLUDE_DIRS})

# Add C library
add_library(watchdog SHARED watchdog.c)

# target_link_libraries(lib ${MSGPUCK_LIBRARIES})
set_target_properties(watchdog PROPERTIES PREFIX "")

# Install module
install(TARGETS watchdog LIBRARY DESTINATION ${TARANTOOL_INSTALL_LIBDIR}/${PROJECT_NAME}/)

enable_testing()
add_test(watchdog_started ${CMAKE_CURRENT_SOURCE_DIR}/test/started.lua)
add_test(watchdog_stopped ${CMAKE_CURRENT_SOURCE_DIR}/test/stopped.lua)
add_test(watchdog_abort ${CMAKE_CURRENT_SOURCE_DIR}/test/abort.sh)
