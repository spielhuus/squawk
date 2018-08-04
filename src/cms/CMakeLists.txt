# -------------------------------------------------------------------------
# Build squawk content directory server app
# -------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.3)

project( cms )

aux_source_directory( ${PROJECT_SOURCE_DIR} CDS_SOURCES )

include_directories( ${IMPORTED_INCLUDES} ${GTEST_INCLUDE_DIR})
add_library(${PROJECT_NAME} ${CDS_SOURCES} ${CDS_SOURCES_MODULES} ${CDS_SOURCES_UTILS})
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS} Threads::Threads -ldl)

add_executable(squawk-${PROJECT_NAME} main.cpp)
target_link_libraries(squawk-${PROJECT_NAME} ${PROJECT_NAME})
