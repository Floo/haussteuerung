# ########## Project setup ##########
#PROJECT(src)
#CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ######### General setup ##########
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/lib/iniparser-2.16/src)


# ########## src executable ##########
# Sources:
SET(src_executable_SRCS
    socket.cpp
    jalousie.cpp
    haussteuerung.cpp
    ds1631_ts.cpp
    max7311_ts.cpp
    temp.cpp
    sharedMem.cpp
    pcf8574_ts.cpp
    abluft.cpp
    devices.cpp
    log.cpp
    e6tcpclient.cpp
    e6udpserver.cpp
    guitcpserver.cpp
)




# actual target:
ADD_EXECUTABLE(haussteuerung ${src_executable_SRCS})
target_link_libraries(haussteuerung ${CMAKE_THREAD_LIBS_INIT} iniparser rt)
#target_link_libraries(haussteuerung ${PTHREADS_LIBRARY} iniparser rt)

# add install target:
INSTALL(TARGETS haussteuerung DESTINATION bin)
