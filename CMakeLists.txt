cmake_minimum_required(VERSION 2.6)

if (POLICY CMP0053)
cmake_policy(SET CMP0053 OLD)
endif ()

if (POLICY CMP0054)
    cmake_policy(SET CMP0054 OLD)
endif ()

project(ucaandor C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(Andor REQUIRED)
find_package(PkgConfig REQUIRED)

include(FindPackageHandleStandardArgs)
include(PkgConfigVars)

add_definitions("-std=c99 -Wall -fPIC")
add_definitions(-DG_LOG_DOMAIN="Uca-Andor")

pkg_check_modules(UCA libuca>=2.1.0 REQUIRED)
pkg_check_variable(libuca plugindir)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${ANDOR_INCLUDE_DIRS}
    ${UCA_INCLUDE_DIRS})

link_directories(
    ${UCA_LIBRARY_DIRS}
    ${ANDOR_LIBRARY_DIRS})

add_library(ucaandor SHARED uca-andor-camera.c)

target_link_libraries(ucaandor
    ${UCA_LIBRARIES}
    ${ANDOR_LIBRARIES})

install(TARGETS ucaandor
        LIBRARY DESTINATION ${LIBUCA_PLUGINDIR}
        RUNTIME DESTINATION ${LIBUCA_PLUGINDIR})
