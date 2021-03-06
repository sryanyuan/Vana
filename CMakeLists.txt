cmake_minimum_required(VERSION 2.6) 

project(Vana)

add_definitions(-std=c++11 -m32 -DDAEMON)

set(LIBRARY_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/build/lib")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/build/bin")

#### Finding libraries
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.54.0 REQUIRED COMPONENTS filesystem system thread)

find_package(MySQL REQUIRED)
include_directories(${MYSQL_INCLUDE_DIR})

find_package(SOCI REQUIRED)
include_directories(${SOCI_CORE_INCLUDE_DIR})
include_directories(${SOCI_MYSQL_INCLUDE_DIR})

find_package(Lua52 REQUIRED)
include_directories(${LUA_INCLUDE_DIR})

find_package(Botan REQUIRED)
include_directories(${BOTAN_INCLUDE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/src/Common)

add_subdirectory(src)
