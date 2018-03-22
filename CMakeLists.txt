project(netconf-cli LANGUAGES CXX)
cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

# Set a default build type if none was specified. This was shamelessly stolen
# from VTK's cmake setup because these guys produce both CMake and a project that
# manipulates this variable, and the web is full of posts where people say that
# it is apparently evil to just set the build type in a way an earlier version of
# this patch did. Oh, and the location of this check/update matters, apparently.
#
# Yes, this is just plain crazy.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# -Werror is not a default for sanity reasons (one cannot know what warnings a future compiler
# might bring along), but it's a default in debug mode. The idea is that developers should care
# about a warning-free build, and that this is easier than messing with yet another configure option.
set(CMAKE_CXX_FLAGS_DEBUG "-Werror ${CMAKE_CXX_FLAGS_DEBUG}")

# I don't want to duplicate the compiler's optimizations
set(CMAKE_CXX_FLAGS "-O2 ${CMAKE_CXX_FLAGS}")

# Build warnings are useful tools (and this project should be warning-free anyway), enable them on all
# configurations. They are warnings, not errors.
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -Wsuggest-override -Woverloaded-virtual ${CMAKE_CXX_FLAGS}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

add_custom_target(git-version-cmake-ide
    cmake/ProjectGitVersion.cmake
    cmake/ProjectGitVersionRunner.cmake
    )
include(cmake/ProjectGitVersion.cmake)
prepare_git_version(NETCONF_CLI_VERSION "0.0")

find_package(Doxygen)
option(WITH_DOCS "Create and install internal documentation (needs Doxygen)" ${DOXYGEN_FOUND})

find_package(docopt REQUIRED)
find_package(spdlog REQUIRED)
find_package(Boost REQUIRED)

set(parser_SRCS
    src/CTree.cpp
    src/CParser.cpp
    src/ast.cpp
    )

add_library(parser STATIC ${parser_SRCS})
target_link_libraries(parser Boost::boost)

set(netconf-cli_SRCS
    src/main.cpp
    )

add_executable(netconf-cli ${netconf-cli_SRCS})
target_link_libraries(netconf-cli docopt parser)
add_dependencies(netconf-cli target-NETCONF_CLI_VERSION)
target_include_directories(netconf-cli PRIVATE ${PROJECT_BINARY_DIR})

include(CTest)
if(BUILD_TESTING)
    enable_testing()
    find_path(TROMPELOEIL_PATH trompeloeil.hpp PATH_SUFFIXES trompeloeil/)
    if("${TROMPELOEIL_PATH}" STREQUAL "TROMPELOEIL_PATH-NOTFOUND")
        message(FATAL_ERROR "Cannot find the \"trompeloeil.hpp\" file provided by <https://github.com/rollbear/trompeloeil>. "
            "Please set TROMPELOEIL_PATH to where it is available.")
    endif()

    find_path(CATCH_PATH catch.hpp PATH_SUFFIXES catch/)
    if("${CATCH_PATH}" STREQUAL "CATCH_PATH-NOTFOUND")
        message(FATAL_ERROR "Cannot find the \"catch.hpp\" file provided by <http://catch-lib.net/>. "
            "Please set CATCH_PATH to where it is available.")
    endif()

    add_library(TestCatchIntegration STATIC
        tests/catch_integration.cpp
        tests/trompeloeil_catch.h
        )
    target_include_directories(TestCatchIntegration SYSTEM PUBLIC ${TROMPELOEIL_PATH} ${CATCH_PATH})
    target_include_directories(TestCatchIntegration PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tests/ ${CMAKE_CURRENT_SOURCE_DIR}/src/)
    target_link_libraries(TestCatchIntegration spdlog::spdlog)

    macro(cli_test fname)
        set(test_${fname}_SOURCES tests/${fname}.cpp)
        add_executable(test_${fname} ${test_${fname}_SOURCES})
        target_link_libraries(test_${fname} TestCatchIntegration parser)
        if(NOT CMAKE_CROSSCOMPILING)
            add_test(test_${fname} test_${fname})
        endif()
        target_include_directories(test_${fname} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
        target_link_libraries(test_${fname} TestCatchIntegration)
    endmacro()
    cli_test(dummy)
    cli_test(cd)

endif()

if(WITH_DOCS)
    set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    configure_file(${doxyfile_in} ${doxyfile} @ONLY)
    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
        SOURCES ${doxyfile_in}
        )
endif()
