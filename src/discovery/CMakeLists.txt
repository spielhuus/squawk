# -------------------------------------------------------------------------
# Build squawk content directory server app
# -------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.3)

project( discovery )

SET(SSDP_SOURCES ssdpserver.cpp ssdpclientconnection.cpp ssdpserverconnection.cpp)

include_directories( ${IMPORTED_INCLUDES} ${GTEST_INCLUDE_DIR})
add_library(${PROJECT_NAME} ${SSDP_SOURCES})
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS} Threads::Threads -ldl)

add_executable(squawk-${PROJECT_NAME} main.cpp)
target_link_libraries(squawk-${PROJECT_NAME} ${PROJECT_NAME} -lstdc++fs)
