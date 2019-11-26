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
find_package(Boost REQUIRED)
find_library(REPLXX_LIBRARY NAMES replxx replxx-d REQUIRED)
find_path(REPLXX_PATH replxx.hxx)
if("${REPLXX_PATH}" STREQUAL REPLXX_PATH-NOTFOUND)
    message(FATAL_ERROR "Cannot find the \"replxx.hxx\" include file for the replxx library.")
endif()

find_package(PkgConfig)
pkg_check_modules(LIBYANG REQUIRED libyang-cpp>=0.15.111)
# TODO: bump to 0.7.8 once it is tagged
pkg_check_modules(SYSREPO REQUIRED libSysrepo-cpp>=0.7.7)
pkg_check_modules(LIBNETCONF2 REQUIRED libnetconf2>=0.12.47)

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

add_library(netconfaccess STATIC
    src/netconf-client.cpp
    src/netconf_access.cpp
    )

target_link_libraries(netconfaccess datastoreaccess ${LIBNETCONF2_LIBRARIES})
link_directories(${LIBNETCONF2_LIBRARY_DIRS})
target_include_directories(netconfaccess SYSTEM PRIVATE ${LIBNETCONF2_INCLUDE_DIRS})

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
target_include_directories(netconf-cli PRIVATE ${REPLXX_PATH})
target_link_libraries(netconf-cli netconfaccess sysrepoaccess yangschema docopt parser ${REPLXX_LIBRARY})
if(CMAKE_CXX_FLAGS MATCHES "-stdlib=libc\\+\\+")
    target_link_libraries(netconf-cli c++experimental)
else()
    target_link_libraries(netconf-cli stdc++fs)
endif()

add_dependencies(netconf-cli target-NETCONF_CLI_VERSION)
target_include_directories(netconf-cli PRIVATE ${PROJECT_BINARY_DIR})

include(CTest)
if(BUILD_TESTING)
    enable_testing()
    find_package(trompeloeil 33 REQUIRED)
    find_package(doctest 2.3.1 REQUIRED)

    add_library(DoctestIntegration STATIC
        tests/doctest_integration.cpp
        tests/trompeloeil_doctest.h
        tests/wait-a-bit-longer.cpp
        )
    target_include_directories(DoctestIntegration PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tests/ ${CMAKE_CURRENT_SOURCE_DIR}/src/)
    target_link_libraries(DoctestIntegration doctest::doctest trompeloeil)
    target_compile_definitions(DoctestIntegration PUBLIC DOCTEST_CONFIG_SUPER_FAST_ASSERTS)

    if (NOT SYSREPOCTL_EXECUTABLE)
        find_program(SYSREPOCTL_EXECUTABLE sysrepoctl)
    endif()
    if (NOT SYSREPOCTL_EXECUTABLE)
        message(FATAL_ERROR "Unable to find sysrepoctl, set SYSREPOCTL_EXECUTABLE manually.")
    endif()

    if (NOT SYSREPOCFG_EXECUTABLE)
        find_program(SYSREPOCFG_EXECUTABLE sysrepocfg)
    endif()
    if (NOT SYSREPOCFG_EXECUTABLE)
        message(FATAL_ERROR "Unable to find sysrepocfg, set SYSREPOCFG_EXECUTABLE manually.")
    endif()

    if (NOT SYSREPOD_EXECUTABLE)
        find_program(SYSREPOD_EXECUTABLE sysrepod)
    endif()
    if (NOT SYSREPOD_EXECUTABLE)
        message(FATAL_ERROR "Unable to find sysrepod, set SYSREPOD_EXECUTABLE manually.")
    endif()

    if (NOT SYSREPO_PLUGIND_EXECUTABLE)
        find_program(SYSREPO_PLUGIND_EXECUTABLE sysrepo-plugind)
    endif()
    if (NOT SYSREPO_PLUGIND_EXECUTABLE)
        message(FATAL_ERROR "Unable to find sysrepo-plugind, set SYSREPO_EXECUTABLE manually.")
    endif()

    if (NOT NETOPEER2_EXECUTABLE)
        find_program(NETOPEER2_EXECUTABLE netopeer2-server)
    endif()
    if (NOT NETOPEER2_EXECUTABLE)
        message(FATAL_ERROR "Unable to find netopeer2-server, set NETOPEER2_EXECUTABLE manually.")
    endif()

    if (NOT FAKEROOT_EXECUTABLE)
        find_program(FAKEROOT_EXECUTABLE fakeroot)
    endif()
    if (NOT FAKEROOT_EXECUTABLE)
        message(FATAL_ERROR "Unable to find fakeroot, set FAKEROOT_EXECUTABLE manually.")
    endif()

    set(NETOPEER_SOCKET_PATH "${CMAKE_CURRENT_BINARY_DIR}/netopeer2-server.sock")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/start_daemons.sh.in ${CMAKE_CURRENT_BINARY_DIR}/start_daemons.sh @ONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/netopeer_vars.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/netopeer_vars.hpp @ONLY)

    function(setup_datastore_tests)
        add_test(NAME setup_netopeer COMMAND ${SYSREPOCFG_EXECUTABLE} ietf-netconf-server -i ${CMAKE_CURRENT_SOURCE_DIR}/netopeer-test-config --datastore=startup --format=xml)
        add_test(NAME start_daemons COMMAND ${FAKEROOT_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/start_daemons.sh)
        add_test(NAME kill_daemons COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/kill_daemons.sh)
        set_tests_properties(setup_netopeer start_daemons kill_daemons PROPERTIES RESOURCE_LOCK sysrepo)
        set_tests_properties(setup_netopeer PROPERTIES FIXTURES_SETUP netopeer_configured)
        set_tests_properties(start_daemons PROPERTIES FIXTURES_REQUIRED netopeer_configured FIXTURES_SETUP netopeer_running)
    endfunction()

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
        target_link_libraries(test_${name} DoctestIntegration parser)
        if(NOT CMAKE_CROSSCOMPILING)
            add_test(test_${name} test_${name})
        endif()
        target_include_directories(test_${name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    endfunction()

    function(datastore_test_impl name model_path backend)
        set(TESTNAME test_${name}_${backend})
        cli_test(${name}_${backend} ${name}.cpp)
        add_test(NAME ${TESTNAME}_init COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/sysrepoctl-manage-module.sh ${SYSREPOCTL_EXECUTABLE} ${SYSREPOCFG_EXECUTABLE} install ${model_path})
        add_test(NAME ${TESTNAME}_cleanup COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/sysrepoctl-manage-module.sh ${SYSREPOCTL_EXECUTABLE} ${SYSREPOCFG_EXECUTABLE} uninstall ${model_path})
        set_tests_properties(${TESTNAME}_init PROPERTIES FIXTURES_REQUIRED netopeer_running FIXTURES_SETUP ${TESTNAME}_setup)
        set_tests_properties(${TESTNAME} PROPERTIES FIXTURES_REQUIRED ${TESTNAME}_setup RESOURCE_LOCK sysrepo)
        set_tests_properties(${TESTNAME}_cleanup PROPERTIES FIXTURES_CLEANUP ${TESTNAME}_setup)
        target_include_directories(${TESTNAME} PRIVATE ${PROJECT_SOURCE_DIR}/tests/mock)
        if (${backend} STREQUAL "sysrepo")
            target_link_libraries(${TESTNAME} sysrepoaccess)
        elseif (${backend} STREQUAL "netconf")
            target_link_libraries(${TESTNAME} netconfaccess)
        else()
            message(FATAL_ERROR "Unknown backend ${backend}")
        endif()
        target_link_libraries(${TESTNAME} yangschema sysreposubscription)

        target_compile_definitions(${TESTNAME} PRIVATE ${backend}_BACKEND)
    endfunction()

    function(datastore_test name model_path)
        set(TESTNAME test_${name})
        datastore_test_impl(${name} ${CMAKE_CURRENT_SOURCE_DIR}/example-schema.yang sysrepo)
        datastore_test_impl(${name} ${CMAKE_CURRENT_SOURCE_DIR}/example-schema.yang netconf)
        set_tests_properties(${TESTNAME}_netconf_init PROPERTIES DEPENDS ${TESTNAME}_sysrepo_cleanup)
    endfunction()

    cli_test(cd)
    cli_test(ls)
    cli_test(presence_containers)
    cli_test(leaf_editing)
    cli_test(yang)
    target_link_libraries(test_yang yangschema)
    cli_test(utils)
    cli_test(path_completion)
    cli_test(command_completion)
    cli_test(enum_completion)
    cli_test(list_manipulation)
    cli_test(parser_methods)

    setup_datastore_tests()
    datastore_test(setting_values ${CMAKE_CURRENT_SOURCE_DIR}/example-schema.yang)
    datastore_test(data_query ${CMAKE_CURRENT_SOURCE_DIR}/example-schema.yang)
    set_tests_properties(kill_daemons PROPERTIES DEPENDS test_setting_values_netconf_cleanup)


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
