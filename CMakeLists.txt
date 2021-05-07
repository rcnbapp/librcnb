###
### CMake settings
###
cmake_minimum_required(VERSION 3.1)

###
### Project settings
###
project(librcnb)

set(PROJECT_VERSION_MAJOR "1")
set(PROJECT_VERSION_MINOR "0")
set(PROJECT_VERSION_PATCH "0")
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_C_FLAGS "/utf-8 ${CMAKE_C_FLAGS}")
endif()

option(ENABLE_AVX2 "Enable AVX2 optimized code." OFF)
option(ENABLE_SSSE3 "Enable SSSE3 optimized code." OFF)
option(NATIVE_ASM "Allow compiler use best instruction set on current environment." OFF)

###
### Sources, headers, directories and libs
###
include_directories(include)

set(RCNB_SOURCES
    src/cencode.c
    src/cdecode.c
    src/rcnb.c
)

if(ENABLE_AVX2)
    if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
        add_compile_options(/arch:AVX2)
    else()
        add_compile_options(-mavx2)
    endif()
    add_compile_definitions(ENABLE_AVX2)
    set(RCNB_SOURCES ${RCNB_SOURCES} src/cencode_x86.c)
elseif(ENABLE_SSSE3)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "MSVC")
        add_compile_options(-mssse3)
    endif()
    add_compile_definitions(ENABLE_SSSE3)
    set(RCNB_SOURCES ${RCNB_SOURCES} src/cencode_x86.c)
endif()

if(NATIVE_ASM)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "MSVC")
        add_compile_options(-march=native)
    endif()
endif()

add_library(rcnb SHARED ${RCNB_SOURCES})
add_library(rcnb-static STATIC ${RCNB_SOURCES})
set_target_properties(rcnb PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER "include/rcnb/cencode.h;include/rcnb/cdecode.h;include/rcnb/encode.h;include/rcnb/decode.h")
set_target_properties(rcnb-static PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER "include/rcnb/cencode.h;include/rcnb/cdecode.h;include/rcnb/encode.h;include/rcnb/decode.h")
if (NOT CMAKE_VERSION VERSION_LESS 2.8.12)
    target_include_directories(rcnb-static
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>)
endif()
add_executable(example1 examples/c-example1.c)
target_link_libraries(example1 rcnb-static)
add_executable(example2 examples/c-example2.c)
target_link_libraries(example2 rcnb-static)
add_executable(example3 examples/cpp-example3.cc)
target_link_libraries(example3 rcnb-static)
add_executable(rcnb-cli examples/cpp-rcnb-cli.cc)
set_target_properties(example3
        PROPERTIES CXX_STANDARD 11)
target_link_libraries(rcnb-cli rcnb-static)
set_target_properties(rcnb-cli
        PROPERTIES OUTPUT_NAME rcnb
        CXX_STANDARD 11)

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
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rcnb)
install(EXPORT ${PROJECT_NAME}-config DESTINATION share/${PROJECT_NAME}/cmake)
