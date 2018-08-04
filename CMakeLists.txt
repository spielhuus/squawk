# -------------------------------------------------------------------------
# Build squawk content directory server
# -------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.3)

project( cds2 )

SET(CDS_TAG_VERSION "master" CACHE STRING "The tagged version.")

# -----------------------   Options and Parameters  -----------------------

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
option(build_tests "Build all squawk unit tests." OFF)

# -----------------------   Config Toolchain        -----------------------

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

#link_libraries("-static")

#find_program(CCACHE_FOUND ccache)
#if(CCACHE_FOUND)
#    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
#    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
#endif(CCACHE_FOUND)

add_definitions( -DHTTP_BUFFER_SIZE=4096 -DHTTP_CLIENT_TIMEOUT=5
                 -DHTTP_SERVER_THREAD_POOL_SIZE=10 -DASIO_STANDALONE
                 -DCDS_VERSION="${CDS_TAG_VERSION}" -DSPDLOG_FMT_EXTERNAL
                 -DFMT_HEADER_ONLY)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DSPDLOG_TRACE_ON)
    add_definitions(-DSPDLOG_DEBUG_ON)
    add_definitions( -DDEBUG )
endif(CMAKE_BUILD_TYPE MATCHES Debug)

# ---------------------   External Dependencies       ---------------------

message(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# -----------------------   Compile Binary          -----------------------

add_subdirectory(src/discovery)
add_subdirectory(src/scanner)
add_subdirectory(src/upnp)
add_subdirectory(src/cms)
add_subdirectory(src/api)
add_subdirectory(src/www)

if (build_tests)
    add_subdirectory(test)
endif()

#aux_source_directory( ${PROJECT_SOURCE_DIR}/src CDS_SOURCES )
#aux_source_directory( src/modules/ CDS_SOURCES_MODULES )
#aux_source_directory( src/utils/ CDS_SOURCES_UTILS )

#include_directories( ${IMPORTED_INCLUDES})
#add_executable(cds ${PROJECT_SOURCE_DIR}/main.cpp)
#target_link_libraries(cds cds_lib ${CONAN_LIBS})
#install(TARGETS cds RUNTIME DESTINATION bin)

# -------------------------------------------------------------------------
# -----------------------   Doxygen                 -----------------------
# -------------------------------------------------------------------------
#find_package(Doxygen)
#if(DOXYGEN_FOUND)
#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
#add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
#    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
#    COMMENT "Generating API documentation with Doxygen" VERBATIM
#)
#endif(DOXYGEN_FOUND)

# -------------------------------------------------------------------------
# -----------------------   Generate Package        -----------------------
# -------------------------------------------------------------------------
IF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
INCLUDE(InstallRequiredSystemLibraries)

SET(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")
SET(CPACK_GENERATOR "TGZ")
SET(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE 1)
SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)

#if(CDS_TAG_VERSION MATCHES "^([0-9]+)[.]([0-9]+)[.]([0-9]+)$" )
#    string(REPLACE "." ";" CDS_TAG_VERSION_STRING ${CDS_TAG_VERSION})
#    list(GET CDS_TAG_VERSION_STRING 0 MAJOR_VERSION)
#    list(GET CDS_TAG_VERSION_STRING 1 MINOR_VERSION)
#    list(GET CDS_TAG_VERSION_STRING 2 PATCH_VERSION)
#else()
#    SET(MAJOR_VERSION "0")
#    SET(MINOR_VERSION "0")
#    SET(PATCH_VERSION "0")
#endif()

SET(CPACK_PACKAGE_VERSION_MAJOR "${MAJOR_VERSION}")
SET(CPACK_PACKAGE_VERSION_MINOR "${MINOR_VERSION}")
SET(CPACK_PACKAGE_VERSION_PATCH "${PATCH_VERSION}")

SET(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}_${CDS_TAG_VERSION}")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}_${CDS_TAG_VERSION}")

install(FILES ${CMAKE_BINARY_DIR}/share/misc/magic.mgc DESTINATION share/misc/magic.mgc )

INCLUDE(CPack)

ENDIF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
