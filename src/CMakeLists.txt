# Copyright (c) 2025 acrion innovations GmbH
# Authors: Stefan Zipproth, s.zipproth@acrion.ch
#
# This file is part of nexuslua, see https://github.com/acrion/nexuslua and https://nexuslua.org
#
# nexuslua is offered under a commercial and under the AGPL license.
# For commercial licensing, contact us at https://acrion.ch/sales. For AGPL licensing, see below.
#
# AGPL licensing:
#
# nexuslua is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# nexuslua is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with nexuslua. If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.25.2)
cmake_policy(VERSION 3.25.2)

option(NEXUSLUA_WITH_OPENSSL "Compile with OpenSSL" ON)

# Build libzip as shared instead of static.
option(NEXUSLUA_LIBZIP_SHARED "Build libzip as shared library" OFF)

# Only build tests when explicitly requested, or when this project is the top-level project.
option(NEXUSLUA_BUILD_TESTS "Build unit tests for nexuslua library" ON)

include(FetchContent)
include(ExternalProject) # For driving Boost bootstrap and 'b2 headers'

# ------------------------------------------------------------------------------
# Bring in shared CMake helpers
# ------------------------------------------------------------------------------

FetchContent_Declare(
    acrion_cmake
    GIT_REPOSITORY https://github.com/acrion/cmake.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(acrion_cmake)
if (NOT acrion_cmake_POPULATED)
    FetchContent_MakeAvailable(acrion_cmake)
endif ()

# ------------------------------------------------------------------------------
# Third-party dependencies via FetchContent
# ------------------------------------------------------------------------------

# toml++
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(tomlplusplus)

# lua
FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG v5.4.8
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(lua)

# cbeam
if (NOT TARGET cbeam)
    FetchContent_Declare(
        cbeam
        GIT_REPOSITORY https://github.com/acrion/Cbeam.git
        GIT_TAG main
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(cbeam)
endif ()

# libzip
if (NEXUSLUA_LIBZIP_SHARED)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
else ()
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
endif ()
set(LIBZIP_DO_INSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_GNUTLS OFF CACHE BOOL "" FORCE)
set(ENABLE_MBEDTLS OFF CACHE BOOL "" FORCE)
set(ENABLE_BZIP2 OFF CACHE BOOL "" FORCE)
set(ENABLE_LZMA OFF CACHE BOOL "" FORCE)
set(ENABLE_ZSTD OFF CACHE BOOL "" FORCE)
set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BUILD_REGRESS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_DOC OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    libzip
    GIT_REPOSITORY https://github.com/nih-at/libzip.git
    GIT_TAG v1.11.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(libzip)

# For static libzip we need PIC when linking into this shared library on *nix.
if (NOT WIN32 AND NOT NEXUSLUA_LIBZIP_SHARED)
    if (TARGET zip)
        set_property(TARGET zip PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif ()
endif ()

# ------------------------------------------------------------------------------
# Boost via FetchContent (classic layout using 'b2 headers')
# ------------------------------------------------------------------------------

# Choose the Boost version here (release archive). Keep unified 'boost/' headers.
set(NEXUSLUA_BOOST_VERSION "1.83.0")
string(REPLACE "." "_" NEXUSLUA_BOOST_VERSION_U "${NEXUSLUA_BOOST_VERSION}")

FetchContent_Declare(
    boost_src
    URL "https://archives.boost.io/release/${NEXUSLUA_BOOST_VERSION}/source/boost_${NEXUSLUA_BOOST_VERSION_U}.tar.gz"
)
FetchContent_GetProperties(boost_src)
if (NOT boost_src_POPULATED)
    FetchContent_MakeAvailable(boost_src)
endif()

# Bootstrap Boost.Build, then generate the unified headers structure (./b2 headers).
if (WIN32)
    set(BOOST_BOOTSTRAP_CMD cmd /c bootstrap.bat)
    set(BOOST_B2_HEADERS_CMD cmd /c .\\b2.exe headers)
else()
    set(BOOST_BOOTSTRAP_CMD ./bootstrap.sh)
    set(BOOST_B2_HEADERS_CMD ./b2 headers)
endif()

ExternalProject_Add(boost_headers
    DOWNLOAD_COMMAND ""
    SOURCE_DIR        ${boost_src_SOURCE_DIR}
    CONFIGURE_COMMAND ${BOOST_BOOTSTRAP_CMD}
    BUILD_COMMAND     ${BOOST_B2_HEADERS_CMD}
    INSTALL_COMMAND   ""
    BUILD_IN_SOURCE   1
)

# Single include root for Boost (contains the 'boost/' folder after 'b2 headers').
set(NEXUSLUA_BOOST_INCLUDE_DIR ${boost_src_SOURCE_DIR})

# ------------------------------------------------------------------------------
# Project versioning and compiler config
# ------------------------------------------------------------------------------

include(${acrion_cmake_SOURCE_DIR}/set-cpp-version.cmake)
include(${acrion_cmake_SOURCE_DIR}/get-project-version-from-git-tag.cmake)
get_project_version_from_git_tag("origin/master")

project(nexuslua_library
    VERSION ${version_major}.${version_minor}.${version_patch}
    DESCRIPTION "nexuslua shared library"
)

# Optional OpenSSL + OpenMP
if (NEXUSLUA_WITH_OPENSSL)
    set(OPENSSL_DEFINE "#define NEXUSLUA_WITH_OPENSSL 1")
else ()
    set(OPENSSL_DEFINE "#define NEXUSLUA_WITH_OPENSSL 0")
endif ()

if (NEXUSLUA_WITH_OPENSSL)
    if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        set(OPENSSL_USE_STATIC_LIBS TRUE)
    endif ()
    find_package(OpenSSL REQUIRED)  # brew install openssl
    set(OPENSSL_TARGETS OpenSSL::SSL OpenSSL::Crypto OpenSSL::applink)
endif ()

include(${acrion_cmake_SOURCE_DIR}/find-openmp.cmake)

# ------------------------------------------------------------------------------
# Generated headers and source organisation
# ------------------------------------------------------------------------------

include(GenerateExportHeader)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.hpp)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.txt.cmake ${CMAKE_CURRENT_BINARY_DIR}/version.txt)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version_nexuslua.hpp.cmake ${CMAKE_CURRENT_BINARY_DIR}/version_nexuslua.hpp)

# Bring in Boost.Filesystem sources directly from the Boost tree fetched above.
# This allows building without linking a separate boost_filesystem library.
file(GLOB boost_filesystem ${boost_src_SOURCE_DIR}/libs/filesystem/src/*.cpp)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lua_find_signature_map.cpp ${CMAKE_CURRENT_BINARY_DIR}/lua_find_signature_if_chain.cpp
    COMMAND lua ${CMAKE_CURRENT_SOURCE_DIR}/lua_code_generator_find_signature.lua ${CMAKE_CURRENT_BINARY_DIR}/lua_find_signature_map.cpp ${CMAKE_CURRENT_BINARY_DIR}/lua_find_signature_if_chain.cpp
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/lua_code_generator_find_signature.lua
)

source_group(boost.org REGULAR_EXPRESSION boost/)
source_group(interface REGULAR_EXPRESSION "interface/")
source_group(generated REGULAR_EXPRESSION ${CMAKE_CURRENT_BINARY_DIR}/)

set(SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/nexuslua_export.h
    ${CMAKE_CURRENT_BINARY_DIR}/config.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/lua_find_signature_if_chain.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/lua_find_signature_map.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/version.txt
    lua_code_generator_find_signature.lua
    config.hpp.cmake
    interface/nexuslua/agent.hpp
    interface/nexuslua/agents.hpp
    interface/nexuslua/agent_message.hpp
    interface/nexuslua/configuration.hpp
    interface/nexuslua/cpp_handler.hpp
    interface/nexuslua/description.hpp
    interface/nexuslua/lua_table.hpp
    interface/nexuslua/message.hpp
    interface/nexuslua/plugin_install_result.hpp
    interface/nexuslua/plugin_registry.hpp
    interface/nexuslua/utility.hpp
    agent.cpp
    agents.cpp
    agent_message.cpp
    agent_cpp.cpp
    agent_cpp.hpp
    agent_id.hpp
    agent_lua.cpp
    agent_lua.hpp
    agent_plugin.cpp
    agent_plugin.hpp
    agent_thread_base.hpp
    agent_thread.hpp
    agent_thread_cpp.cpp
    agent_thread_cpp.hpp
    agent_thread_lua.cpp
    agent_thread_lua.hpp
    description.cpp
    lua_call_info.cpp
    lua_call_info.hpp
    lua_find_signature.hpp
    lua_extension.cpp
    lua_extension.hpp
    lua_table.cpp
    lua.cpp
    lua.hpp
    main.cpp
    message.cpp
    message_counter.hpp
    message_to_agent.hpp
    platform_specific.cpp
    platform_specific.hpp
    plugin_registry.cpp
    plugin_spec.cpp
    plugin_spec.hpp
    thread_pool.hpp
    utility.cpp
    version.txt.cmake
    ${boost_filesystem}
)

foreach (source IN LISTS boost_filesystem)
    if (MSVC)
        set_source_files_properties(${source} PROPERTIES COMPILE_FLAGS "/W0")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set_source_files_properties(${source} PROPERTIES COMPILE_FLAGS "-w")
        set_source_files_properties(${source} PROPERTIES LINK_FLAGS "-Wl,--no-warn-search-mismatch")
    endif ()
endforeach ()

# ------------------------------------------------------------------------------
# Library target
# ------------------------------------------------------------------------------

if (WIN32) # Visual Studio and MinGW
    add_library(${PROJECT_NAME} SHARED ${SRCS} resource.rc)
else ()   # g++ under Mac and Linux
    add_library(${PROJECT_NAME} SHARED ${SRCS})
endif ()

# Make sure Boost headers are prepared before building this target.
add_dependencies(${PROJECT_NAME} boost_headers)

# Provide the canonical alias for consumers using FetchContent.
# Users can: target_link_libraries(their_app PRIVATE nexuslua::nexuslua)
add_library(nexuslua::nexuslua ALIAS ${PROJECT_NAME})

include(${acrion_cmake_SOURCE_DIR}/set-compiler-flags.cmake)
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_compile_definitions(${PROJECT_NAME} PRIVATE BOOST_FILESYSTEM_NO_CXX20_ATOMIC_REF)
endif ()

# Ensure Boost.System is header-only to avoid linking a separate library.
target_compile_definitions(${PROJECT_NAME} PRIVATE BOOST_ERROR_CODE_HEADER_ONLY)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "nexuslua")

generate_export_header(${PROJECT_NAME} BASE_NAME nexuslua)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/interface
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/clime
        ${CMAKE_CURRENT_BINARY_DIR}
    PRIVATE
        ${lua_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/interface/nexuslua
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${NEXUSLUA_BOOST_INCLUDE_DIR} # Single include root containing the 'boost/' headers
)

# Linking
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    add_definitions(-DBOOST_ALL_NO_LIB)
    target_link_libraries(${PROJECT_NAME} PUBLIC cbeam PRIVATE zip tomlplusplus::tomlplusplus lua OpenMP::OpenMP_CXX ${OPENSSL_TARGETS} version)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)

    if (WIN32) # MinGW (msys2/ucrt64)
        target_link_libraries(${PROJECT_NAME} PUBLIC cbeam PRIVATE zip tomlplusplus::tomlplusplus lua OpenMP::OpenMP_CXX ${OPENSSL_TARGETS} Threads::Threads stdc++fs libversion.a ws2_32)
    elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        target_link_libraries(${PROJECT_NAME} PUBLIC cbeam PRIVATE zip tomlplusplus::tomlplusplus lua OpenMP::OpenMP_CXX ${OPENSSL_TARGETS} Threads::Threads stdc++fs)
    elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_link_libraries(${PROJECT_NAME} PUBLIC cbeam PRIVATE zip tomlplusplus::tomlplusplus lua OpenMP::OpenMP_CXX ${OPENSSL_TARGETS} Threads::Threads)
    endif ()
else ()
    target_link_libraries(${PROJECT_NAME} PUBLIC cbeam PRIVATE zip tomlplusplus::tomlplusplus lua OpenMP::OpenMP_CXX ${OPENSSL_TARGETS})
endif ()

include(${acrion_cmake_SOURCE_DIR}/do-logging.cmake)

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

if (NEXUSLUA_BUILD_TESTS)
    project(nexuslua_test VERSION ${version_major}.${version_minor}.${version_patch} DESCRIPTION "unit test executable for nexuslua library") # required for below run-tests.cmake

    find_package(Threads REQUIRED)
    include(${acrion_cmake_SOURCE_DIR}/find-openmp.cmake)

    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
        GIT_SHALLOW TRUE
    )
    if (WIN32)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif ()
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_executable(
        ${PROJECT_NAME}
        test/test_configuration.cpp
        test/test_extensions.cpp
        test/test_lua.cpp
        test/test_message.cpp
    )

    # Ensure Boost headers are prepared before building tests, too.
    add_dependencies(${PROJECT_NAME} boost_headers)

    target_include_directories(
        ${PROJECT_NAME}
        PRIVATE
        ${nexuslua_SOURCE_DIR} # access internal headers of nexuslua library
        ${nexuslua_SOURCE_DIR}/interface/nexuslua
    )

    target_link_libraries(
        ${PROJECT_NAME}
        nexuslua_library
        Threads::Threads
        GTest::gtest_main
        OpenMP::OpenMP_CXX
    )

    include(GoogleTest)
    # gtest_discover_tests(${PROJECT_NAME})

    add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/testing-resources"
        "$<TARGET_FILE_DIR:nexuslua_test>/nexuslua-library-testing-resources"
        COMMENT "Copying testing resources for nexuslua_test"
    )

    include(${acrion_cmake_SOURCE_DIR}/run-tests.cmake)
endif ()
