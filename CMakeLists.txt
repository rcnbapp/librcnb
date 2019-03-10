###
### CMake settings
###
cmake_minimum_required(VERSION 3.1)

###
### Project settings
###
project(librcnb C)

set(PROJECT_VERSION_MAJOR "1")
set(PROJECT_VERSION_MINOR "0")
set(PROJECT_VERSION_PATCH "0")
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

###
### Sources, headers, directories and libs
###
include_directories(include)

add_library(rcnb SHARED src/cencode.c src/cdecode.c src/rcnb.c)
add_library(rcnb-static STATIC src/cencode.c src/cdecode.c src/rcnb.c)
set_target_properties(rcnb PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER include/rcnb/cencode.h
        PUBLIC_HEADER include/rcnb/cdecode.h)
set_target_properties(rcnb-static PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER include/rcnb/cencode.h
        PUBLIC_HEADER include/rcnb/cdecode.h)

add_executable(example1 examples/c-example1.c)
target_link_libraries(example1 rcnb-static)
add_executable(example2 examples/c-example2.c)
target_link_libraries(example2 rcnb-static)

###
### General compilation settings
###
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

###
### General install settings
###
include(GNUInstallDirs)

export(
        TARGETS rcnb-static
        FILE "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-targets.cmake")
export(PACKAGE ${PROJECT_NAME})
set(EXPORT_TARGETS rcnb-static CACHE INTERNAL "export targets")

set(CONFIG_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}-config.cmake.in
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake" @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}-config-version.cmake.in
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake" @ONLY)

install(TARGETS rcnb EXPORT ${PROJECT_NAME}-config
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
install(EXPORT ${PROJECT_NAME}-config DESTINATION share/${PROJECT_NAME}/cmake)
