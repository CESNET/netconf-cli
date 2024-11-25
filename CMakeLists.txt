cmake_minimum_required(VERSION 3.19)
project(netconf-cli LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

include(GNUInstallDirs)
include(CTest)

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
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -Woverloaded-virtual -Wimplicit-fallthrough ${CMAKE_CXX_FLAGS}")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "-Wsuggest-override ${CMAKE_CXX_FLAGS}")
endif()

add_custom_target(git-version-cmake-ide
    cmake/ProjectGitVersion.cmake
    cmake/ProjectGitVersionRunner.cmake
    cmake/DiscoverNetconfExecutables.cmake
    )
include(cmake/ProjectGitVersion.cmake)
prepare_git_version(NETCONF_CLI_VERSION "1")

# Boost.Process needs linking with threads, but doesn't have special CMake target which would do that for us. So, we
# need to link manually. This will hopefully change in the future.
# https://discourse.cmake.org/t/boost-process-target-doesnt-exist-for-thread-linking/2113
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

find_package(Doxygen)
option(WITH_DOCS "Create and install internal documentation (needs Doxygen)" ${DOXYGEN_FOUND})

find_package(Boost REQUIRED CONFIG COMPONENTS filesystem)
# Fixes C++20 build
# https://github.com/boostorg/asio/issues/312
add_definitions(-DBOOST_ASIO_DISABLE_CONCEPTS)
find_package(replxx REQUIRED)

find_package(PkgConfig)
pkg_check_modules(DOCOPT REQUIRED IMPORTED_TARGET docopt)

set(ENABLE_SYSREPO_CLI AUTO CACHE STRING "Enable the `sysrepo-cli`")
set_property(CACHE ENABLE_SYSREPO_CLI PROPERTY STRINGS AUTO ON OFF)
set(ENABLE_FULL_TESTS AUTO CACHE STRING "Enable end-to-end tests via sysrepo and netopeer2")
set_property(CACHE ENABLE_FULL_TESTS PROPERTY STRINGS AUTO ON OFF)
if(NOT (ENABLE_SYSREPO_CLI STREQUAL ON OR ENABLE_SYSREPO_CLI STREQUAL OFF OR ENABLE_SYSREPO_CLI STREQUAL AUTO))
    message(FATAL_ERROR "ENABLE_SYSREPO_CLI must be one of ON, OFF, AUTO")
endif()
if(NOT (ENABLE_FULL_TESTS STREQUAL ON OR ENABLE_FULL_TESTS STREQUAL OFF OR ENABLE_FULL_TESTS STREQUAL AUTO))
    message(FATAL_ERROR "ENABLE_FULL_TESTS must be one of ON, OFF, AUTO")
endif()
if(NOT BUILD_TESTING AND ENABLE_FULL_TESTS STREQUAL ON)
    message(FATAL_ERROR "Cannot combine ENABLE_FULL_TESTS=ON with BUILD_TESTING=OFF")
endif()

pkg_check_modules(LIBYANG-CPP REQUIRED IMPORTED_TARGET libyang-cpp>=3)
pkg_check_modules(LIBNETCONF2-CPP REQUIRED IMPORTED_TARGET libnetconf2-cpp>=3)

if(ENABLE_FULL_TESTS STREQUAL OFF AND ENABLE_SYSREPO_CLI STREQUAL OFF)
    message(STATUS "Skipping sysrepo per configure options")
    set(SYSREPO_FOUND 0)
else()
    pkg_check_modules(SYSREPO IMPORTED_TARGET sysrepo-cpp>=3)
endif()

if(SYSREPO_FOUND)
    if(ENABLE_SYSREPO_CLI STREQUAL OFF)
        message(STATUS "Skipping the `sysrepo-cli` per config option")
        set(DO_ENABLE_SYSREPO_CLI OFF)
    else()
        message(STATUS "The `sysrepo-cli` will be available")
        set(DO_ENABLE_SYSREPO_CLI ON)
    endif()

    if(BUILD_TESTING)
        include(cmake/DiscoverNetconfExecutables.cmake)
        discover_netconf_executables()

        if(ENABLE_FULL_TESTS STREQUAL OFF)
            message(STATUS "End-to-end NETCONF test suite disabled per config option")
            set(DO_ENABLE_NETCONF_TESTS OFF)
        else()
            message(STATUS "End-to-end NETCONF test suite enabled")
            set(DO_ENABLE_NETCONF_TESTS ON)
        endif()
    endif()
else()
    if(ENABLE_SYSREPO_CLI STREQUAL ON)
        message(FATAL_ERROR "Cannot find sysrepo-cpp which is required for the `sysrepo-cli`")
    elseif(ENABLE_SYSREPO_CLI STREQUAL AUTO)
        message(STATUS "Cannot find sysrepo-cpp, skipping the `sysrepo-cli`")
    endif()
    set(DO_ENABLE_SYSREPO_CLI OFF)

    if(BUILD_TESTING)
        if(ENABLE_FULL_TESTS STREQUAL ON)
            message(FATAL_ERROR "Cannot find sysrepo-cpp which is required for the end-to-end NETCONF test suite")
        elseif(ENABLE_FULL_TESTS STREQUAL AUTO)
            message(STATUS "Cannot find sysrepo-cpp, skipping the end-to-end NETCONF test suite")
        endif()
        set(DO_ENABLE_NETCONF_TESTS OFF)
    endif()
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)

add_library(ast_values STATIC
    src/ast_values.cpp
    )
target_link_libraries(ast_values PUBLIC Boost::boost)

add_library(path STATIC
    src/ast_path.cpp
    )
target_link_libraries(path PUBLIC ast_values Boost::boost)

add_library(leaf_data_type STATIC
    src/leaf_data_type.cpp
    )
target_link_libraries(leaf_data_type PUBLIC ast_values)

add_library(utils STATIC
    src/utils.cpp
    )
target_link_libraries(utils PUBLIC path ast_values)

add_library(schemas STATIC
    src/static_schema.cpp
    src/schema.cpp
    )
target_link_libraries(schemas PUBLIC path leaf_data_type Boost::boost)

add_library(datastoreaccess STATIC
    src/datastore_access.cpp
    src/data_query.cpp
    )
target_link_libraries(datastoreaccess PUBLIC Boost::boost)

if(DO_ENABLE_SYSREPO_CLI OR DO_ENABLE_NETCONF_TESTS)
    add_library(sysrepoaccess STATIC
        src/sysrepo_access.cpp
        )

    target_link_libraries(sysrepoaccess PUBLIC datastoreaccess ast_values PkgConfig::SYSREPO PRIVATE PkgConfig::LIBYANG-CPP)
endif()

add_library(netconfaccess STATIC
    src/netconf_access.cpp
    )

target_link_libraries(netconfaccess PUBLIC datastoreaccess yangschema ast_values utils PkgConfig::LIBNETCONF2-CPP PkgConfig::LIBYANG-CPP)

add_library(yangaccess STATIC
    src/yang_access.cpp
    )

target_link_libraries(yangaccess PUBLIC datastoreaccess yangschema PRIVATE PkgConfig::LIBYANG-CPP)

add_library(yangutils STATIC
    src/libyang_utils.cpp
    )
target_link_libraries(yangutils PUBLIC PkgConfig::LIBYANG-CPP)

add_library(yangschema STATIC
    src/yang_schema.cpp
    )
target_link_libraries(yangschema PUBLIC schemas utils yangutils PRIVATE PkgConfig::LIBYANG-CPP)

add_library(parser STATIC
    src/parser.cpp
    src/ast_commands.cpp
    src/parser_context.cpp
    src/interpreter.cpp
    src/ast_handlers.cpp
    src/completion.cpp
    )
target_link_libraries(parser PUBLIC schemas utils ast_values yangutils)

add_library(proxydatastore STATIC
    src/proxy_datastore.cpp
    )
target_link_libraries(proxydatastore PUBLIC datastoreaccess yangaccess)

# Links libraries, that aren't specific to a datastore type
function(cli_link_required cli_target)
    target_link_libraries(${cli_target} PRIVATE proxydatastore yangschema PkgConfig::DOCOPT parser replxx::replxx)
    add_dependencies(${cli_target} target-NETCONF_CLI_VERSION)
    target_include_directories(${cli_target} PRIVATE ${PROJECT_BINARY_DIR})
endfunction()

if(DO_ENABLE_SYSREPO_CLI)
    add_executable(sysrepo-cli
        src/cli.cpp
        )
    target_compile_definitions(sysrepo-cli PRIVATE SYSREPO_CLI)
    target_link_libraries(sysrepo-cli PUBLIC sysrepoaccess)
    cli_link_required(sysrepo-cli)
    list(APPEND cli_targets sysrepo-cli)
endif()

add_executable(yang-cli
    src/cli.cpp
    )
target_compile_definitions(yang-cli PRIVATE YANG_CLI)
cli_link_required(yang-cli)
target_link_libraries(yang-cli PRIVATE yangaccess)
list(APPEND cli_targets yang-cli)
if(CMAKE_CXX_FLAGS MATCHES "-stdlib=libc\\+\\+" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(yang-cli PRIVATE c++experimental)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.1)
    target_link_libraries(yang-cli PRIVATE stdc++fs)
endif()

add_executable(netconf-cli
    src/cli.cpp
    src/cli-netconf.cpp
    )
target_compile_definitions(netconf-cli PRIVATE NETCONF_CLI)

target_link_libraries(netconf-cli PRIVATE netconfaccess Threads::Threads Boost::filesystem)
cli_link_required(netconf-cli)
list(APPEND cli_targets netconf-cli)


if(BUILD_TESTING)
    find_package(trompeloeil 45 REQUIRED)
    find_package(doctest 2.4.11 REQUIRED)

    add_library(DoctestIntegration STATIC
        tests/doctest_integration.cpp
        tests/trompeloeil_doctest.hpp
        tests/wait-a-bit-longer.cpp
        )
    target_include_directories(DoctestIntegration PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tests/ ${CMAKE_CURRENT_SOURCE_DIR}/src/)
    target_link_libraries(DoctestIntegration PUBLIC doctest::doctest trompeloeil::trompeloeil)
    target_compile_definitions(DoctestIntegration PUBLIC DOCTEST_CONFIG_SUPER_FAST_ASSERTS)

    if(DO_ENABLE_NETCONF_TESTS)
        add_library(sysreposubscription STATIC
            tests/mock/sysrepo_subscription.cpp
            )
        target_link_libraries(sysreposubscription PUBLIC datastoreaccess PkgConfig::SYSREPO)

        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_repositories)
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_netopeer_files)
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_netopeer_outputs)
    endif()

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tests/yang_access_test_vars.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/yang_access_test_vars.hpp @ONLY)

    function(cli_test name)
        if (${ARGC} GREATER 1) # this is how CMake does optional arguments
            add_executable(test_${name}
                tests/${ARGV1}
                )
        else()
            add_executable(test_${name}
                tests/${name}.cpp
                )
        endif()
        target_link_libraries(test_${name} PRIVATE DoctestIntegration parser datastoreaccess)
        if(NOT CMAKE_CROSSCOMPILING)
            add_test(test_${name} test_${name})
        endif()
        target_include_directories(test_${name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    endfunction()

    function(datastore_test_impl name model backend)
        set(TESTNAME test_${name}_${backend})

        cli_test(${name}_${backend} ${name}.cpp)
        target_include_directories(${TESTNAME} PRIVATE ${PROJECT_SOURCE_DIR}/tests/mock)
        if (${backend} STREQUAL "sysrepo")
            target_link_libraries(${TESTNAME} PRIVATE sysrepoaccess)
        elseif (${backend} STREQUAL "netconf")
            target_link_libraries(${TESTNAME} PRIVATE netconfaccess)
        elseif (${backend} STREQUAL "yang")
            target_link_libraries(${TESTNAME} PRIVATE yangaccess)
        else()
            message(FATAL_ERROR "Unknown backend ${backend}")
        endif()
        target_link_libraries(${TESTNAME} PRIVATE yangschema sysreposubscription proxydatastore PkgConfig::SYSREPO)

        target_compile_definitions(${TESTNAME} PRIVATE ${backend}_BACKEND)
        set_tests_properties(${TESTNAME} PROPERTIES FIXTURES_REQUIRED ${TESTNAME}_setup)

        add_test(NAME ${TESTNAME}_init COMMAND ${CMAKE_CURRENT_BINARY_DIR}/init_datastore.bash "${model}" "${backend}")
        set_tests_properties(${TESTNAME}_init PROPERTIES FIXTURES_SETUP ${TESTNAME}_setup)
        add_test(NAME ${TESTNAME}_cleanup COMMAND ${CMAKE_CURRENT_BINARY_DIR}/cleanup_datastore.bash "${backend}")
        set_tests_properties(${TESTNAME}_cleanup PROPERTIES FIXTURES_CLEANUP ${TESTNAME}_setup)

        set_property(TEST ${TESTNAME} ${TESTNAME}_init ${TESTNAME}_cleanup APPEND PROPERTY ENVIRONMENT
            "SYSREPO_REPOSITORY_PATH=${CMAKE_CURRENT_BINARY_DIR}/test_repositories/${TESTNAME}"
            "SYSREPO_SHM_PREFIX=netconf-cli_${TESTNAME}"
            "NETOPEER_SOCKET=test_netopeer_files/${TESTNAME}.sock"
            )

    endfunction()

    function(datastore_test name model)
        datastore_test_impl(${name} ${model} sysrepo)
        datastore_test_impl(${name} ${model} netconf)
        datastore_test_impl(${name} ${model} yang)
    endfunction()

    cli_test(cd)
    cli_test(ls)
    cli_test(presence_containers)
    cli_test(leaf_editing)
    target_link_libraries(test_leaf_editing PRIVATE leaf_data_type)
    cli_test(yang)
    target_link_libraries(test_yang PRIVATE yangschema)
    cli_test(utils)
    target_link_libraries(test_utils PRIVATE yangutils)
    cli_test(path_completion)
    cli_test(command_completion)
    cli_test(set_value_completion)
    target_link_libraries(test_set_value_completion PRIVATE leaf_data_type)
    cli_test(list_manipulation)
    cli_test(interpreter)
    target_link_libraries(test_interpreter PRIVATE proxydatastore)
    cli_test(path_utils)
    target_link_libraries(test_path_utils PRIVATE path)
    cli_test(keyvalue_completion)
    cli_test(parser_rpc)
    cli_test(misc_commands)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tests/init_datastore.bash.in
        ${CMAKE_CURRENT_BINARY_DIR}/init_datastore.bash @ONLY)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tests/cleanup_datastore.bash.in
        ${CMAKE_CURRENT_BINARY_DIR}/cleanup_datastore.bash @ONLY)

    if(DO_ENABLE_NETCONF_TESTS)
        datastore_test(datastore_access ${CMAKE_CURRENT_SOURCE_DIR}/tests/example-schema.yang)
        datastore_test(data_query ${CMAKE_CURRENT_SOURCE_DIR}/tests/example-schema.yang)
    endif()
endif()

option(WITH_PYTHON_BINDINGS "Create and install Python3 bindings for accessing datastores" OFF)
if(WITH_PYTHON_BINDINGS)
    set(PYBIND11_CPP_STANDARD -std=c++20)
    find_package(pybind11 REQUIRED)
    pybind11_add_module(netconf_cli_py src/python_netconf.cpp)
    target_link_libraries(netconf_cli_py PUBLIC netconfaccess)

    if(BUILD_TESTING)
        pybind11_add_module(sysrepo_subscription_py tests/mock/sysrepo_subscription_python.cpp)
        target_link_libraries(sysrepo_subscription_py PUBLIC sysreposubscription utils)

        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tests/python_netconfaccess.py
            ${CMAKE_CURRENT_BINARY_DIR}/tests_python_netconfaccess.py COPYONLY)

        add_test(NAME test_netconf_cli_py COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/tests_python_netconfaccess.py)
        set_tests_properties(test_netconf_cli_py PROPERTIES FIXTURES_REQUIRED test_netconf_cli_py_setup)

        add_test(NAME test_netconf_cli_py_init COMMAND ${CMAKE_CURRENT_BINARY_DIR}/init_datastore.bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/example-schema.yang "netconf")
        set_tests_properties(test_netconf_cli_py_init PROPERTIES FIXTURES_SETUP test_netconf_cli_py_setup)
        add_test(NAME test_netconf_cli_py_cleanup COMMAND ${CMAKE_CURRENT_BINARY_DIR}/cleanup_datastore.bash "netconf")
        set_tests_properties(test_netconf_cli_py_cleanup PROPERTIES FIXTURES_CLEANUP test_netconf_cli_py_setup)

        set_property(TEST test_netconf_cli_py test_netconf_cli_py_init test_netconf_cli_py_cleanup APPEND PROPERTY ENVIRONMENT
            "SYSREPO_REPOSITORY_PATH=${CMAKE_CURRENT_BINARY_DIR}/test_repositories/test_netconf_cli_py"
            "SYSREPO_SHM_PREFIX=netconf-cli_test_netconf_cli_py"
            "NETOPEER_SOCKET=test_netopeer_files/test_netconf_cli_py.sock"
            )

        set(sanitizer_active OFF)
        # FIXME: this just sucks. The detection is very unreliable (one could use something like
        # -fsanitize=address,undefined and we are screwed), and especially clang's query for preload
        # is obviously unportable because we hardcode host's architecture.
        # This is super-ugly. Perhaps it would be better just to outright disable everything, but hey,
        # I need to test this on my laptop where I'm using ASAN by default, and it kinda-almost-works
        # there with just one patch to libyang :).
        if (${CMAKE_CXX_FLAGS} MATCHES "-fsanitize=address")
            set(sanitizer_active ON)
            set(gcc_sanitizer_preload libasan.so)
            set(clang_sanitizer_preload libclang_rt.asan-x86_64.so)
        endif()

        if (sanitizer_active)
            if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
                execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=${clang_sanitizer_preload}
                    OUTPUT_VARIABLE LIBxSAN_FULL_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
            elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=${gcc_sanitizer_preload}
                    OUTPUT_VARIABLE LIBxSAN_FULL_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
            else()
                message(ERROR "Cannot determine correct sanitizer library for LD_PRELOAD")
            endif()
            set_property(TEST test_netconf_cli_py APPEND PROPERTY ENVIRONMENT
                LD_PRELOAD=${LIBxSAN_FULL_PATH}
                ASAN_OPTIONS=detect_leaks=0 # they look harmless, but they are annoying
                )
        endif()
    endif()
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

install(TARGETS ${cli_targets}
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/)
