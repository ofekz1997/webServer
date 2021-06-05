cmake_minimum_required(VERSION 3.0.0)
project(webServer VERSION 0.1.0)

include(CTest)
enable_testing()

#set  (CMAKE_C_FLAGS "")
#string(APPEND CMAKE_C_FLAGS " -lpthread")
add_executable(webServer  server.c  segel.c request.c list.c threadPool.c)

TARGET_LINK_LIBRARIES( webServer pthread)

#  client.c output.c
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
