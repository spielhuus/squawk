# -------------------------------------------------------------------------
# Build squawk content directory server app
# -------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.3)

project( api )

SET(CDS_API_SOURCES apiserver.cpp)

include_directories( ${IMPORTED_INCLUDES} ${GTEST_INCLUDE_DIR})
add_library(${PROJECT_NAME} ${CDS_API_SOURCES})
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS} Threads::Threads -ldl)

add_executable(squawk-api main.cpp)
target_link_libraries(squawk-api ${PROJECT_NAME})

