cmake_minimum_required(VERSION 2.6)
project(Haussteuerung)
set(CMAKE_CXX_FLAGS "-g -Wall -D_REENTRANT")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
find_package(Threads REQUIRED)

#set(PTHREADS_INCLUDE_DIR "/usr/lib")
#set(PTHREADS_INCLUDE_DIR "/usr/lib/x86_64-linux-gnu")

#find_package(RT REQUIRED)

include_directories(${PTHREADS_INCLUDE_DIR}  ${CMAKE_BINARY_DIR}/lib/iniparser-2.16/src)
include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/lib/iniparser-2.16/src)


add_subdirectory(lib/iniparser-2.16/src)
add_subdirectory(src)


file(GLOB HEADERS include/*.h config/*.conf)
file(GLOB SOURCES src/*.cpp)
add_library(includes ${HEADERS} ${SOURCES})

