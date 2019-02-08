project(netconf-cli LANGUAGES CXX)
cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 17)
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
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -Woverloaded-virtual ${CMAKE_CXX_FLAGS}")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "-Wsuggest-override ${CMAKE_CXX_FLAGS}")
endif()

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

find_package(PkgConfig)
pkg_check_modules(LIBYANG REQUIRED libyang-cpp>=0.15.111)
pkg_check_modules(SYSREPO REQUIRED libSysrepo-cpp>=0.7.3)

# we don't need filename tracking, and we prefer to use header-only Boost
add_definitions(-DBOOST_SPIRIT_X3_NO_FILESYSTEM)

add_library(schemas STATIC
    src/static_schema.cpp
    src/schema.cpp
    )
target_link_libraries(schemas PUBLIC Boost::boost)

add_library(datastoreaccess STATIC
    src/datastore_access.cpp
    )
target_link_libraries(datastoreaccess PUBLIC Boost::boost)

add_library(sysrepoaccess STATIC
    src/sysrepo_access.cpp
    )
target_link_libraries(sysrepoaccess datastoreaccess ${SYSREPO_LIBRARIES})
link_directories(${SYSREPO_LIBRARY_DIRS})
target_include_directories(sysrepoaccess SYSTEM PRIVATE ${SYSREPO_INCLUDE_DIRS})

add_library(yangschema STATIC
    src/yang_schema.cpp
    )
target_link_libraries(yangschema ${LIBYANG_LIBRARIES})
# Ensure that this doesn't override Boost's -isystem -- see the log for details.
target_include_directories(yangschema SYSTEM PRIVATE ${LIBYANG_INCLUDEDIR})
link_directories(${LIBYANG_LIBRARY_DIRS})

add_library(parser STATIC
    src/parser.cpp
    src/ast_commands.cpp
    src/ast_path.cpp
    src/utils.cpp
    src/parser_context.cpp
    src/interpreter.cpp
    src/ast_handlers.cpp
    )
target_link_libraries(parser schemas)


add_library(sysreposubscription STATIC
    tests/mock/sysrepo_subscription.cpp
    )

target_link_libraries(sysreposubscription ${SYSREPO_LIBRARIES})
link_directories(${SYSREPO_LIBRARY_DIRS})
target_include_directories(sysreposubscription SYSTEM PRIVATE ${SYSREPO_INCLUDE_DIRS})

add_executable(netconf-cli
    src/main.cpp
    )
target_link_libraries(netconf-cli sysrepoaccess yangschema docopt parser)
if(CMAKE_CXX_FLAGS MATCHES "-stdlib=libc\\+\\+")
    target_link_libraries(netconf-cli c++experimental)
else()
    target_link_libraries(netconf-cli stdc++fs)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/linenoise)

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

    if (NOT SYSREPOCTL_EXECUTABLE)
        find_program(SYSREPOCTL_EXECUTABLE sysrepoctl)
    endif()
    if (NOT SYSREPOCTL_EXECUTABLE)
        message(FATAL_ERROR "Unable to find sysrepoctl, set SYSREPOCTL_EXECUTABLE manually.")
    endif()

    configure_file("${PROJECT_SOURCE_DIR}/sysrepo_vars.hpp.in" "${PROJECT_BINARY_DIR}/sysrepo_vars.hpp" @ONLY)
    configure_file("${PROJECT_SOURCE_DIR}/example-schema.yang" "${PROJECT_BINARY_DIR}/example-schema.yang" COPYONLY)

    function(cli_test fname)
        add_executable(test_${fname}
            tests/${fname}.cpp
            )
        target_link_libraries(test_${fname} TestCatchIntegration parser)
        if(NOT CMAKE_CROSSCOMPILING)
            add_test(test_${fname} test_${fname})
        endif()
        target_include_directories(test_${fname} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
        target_link_libraries(test_${fname} TestCatchIntegration)
    endfunction()
    cli_test(cd)
    cli_test(ls)
    cli_test(presence_containers)
    cli_test(leaf_editing)
    cli_test(yang)
    target_link_libraries(test_yang yangschema)
    cli_test(sysrepo)
    target_link_libraries(test_sysrepo sysreposubscription sysrepoaccess yangschema parser)
    target_include_directories(test_sysrepo PRIVATE ${PROJECT_SOURCE_DIR}/tests/mock)
    cli_test(utils)
    cli_test(path_completion)
    cli_test(command_completion)
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

install(TARGETS
    netconf-cli
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/)
