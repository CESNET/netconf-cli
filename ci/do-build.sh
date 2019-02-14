#!/bin/bash

set -eux -o pipefail
shopt -s failglob

# We're reusing our artifacts, so we absolutely need a stable destdir.
# Turbo-hipster takes care of cleaning up the mess betweeb builds.
PREFIX=~/target
mkdir ${PREFIX}
export PATH=${PREFIX}/bin:$PATH
export LD_LIBRARY_PATH=${PREFIX}/lib64:${PREFIX}/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
export PKG_CONFIG_PATH=${PREFIX}/lib64/pkgconfig:${PREFIX}/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}

if [[ $TH_JOB_NAME =~ .*-sanitizers-.* ]]; then
    CMAKE_OPTIONS="${CMAKE_OPTIONS} -DUSE_SR_MEM_MGMT:BOOL=OFF"
    # https://gitlab.kitware.com/cmake/cmake/issues/16609
    CMAKE_OPTIONS="${CMAKE_OPTIONS} -DTHREADS_HAVE_PTHREAD_ARG:BOOL=ON"
fi

# force-enable tests for packages which use, eh, interesting setup
# - libyang and libnetconf2 copmare CMAKE_BUILD_TYPE to lowercase "debug"...
CMAKE_OPTIONS="${CMAKE_OPTIONS} -DENABLE_BUILD_TESTS=ON -DENABLE_VALGRIND_TESTS=OFF"

# nuke python2 builds because we cannot write to the site_path
CMAKE_OPTIONS="${CMAKE_OPTIONS} -DGEN_PYTHON_BINDINGS=OFF"

build_dep_cmake() {
    pushd ${TH_JOB_WORKING_DIR}
    mkdir build-$1
    pushd build-$1
    ${CMAKE} -GNinja ${CMAKE_OPTIONS} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Debug} -DCMAKE_INSTALL_PREFIX=${PREFIX} ${TH_GIT_PATH}/submodules/$1
    ninja-build install
    popd
    popd
}

build_dep_autoconf() {
    pushd ${TH_GIT_PATH}/submodules/$1
    shift
    ./configure --prefix=${PREFIX} "$@"
    make -j4
    make install
    popd
}

do_test_dep_cmake() {
    pushd ${TH_JOB_WORKING_DIR}/build-$1
    shift
    ${CTEST} --output-on-failure "$@"
    popd
}

emerge_dep() {
    if [[ -f ${TH_GIT_PATH}/submodules/$1/CMakeLists.txt ]]; then
        build_dep_cmake $1
    elif [[ -f ${TH_GIT_PATH}/submodules/$1/configure ]]; then
        build_dep_autoconf "$@"
    else
        echo "Unrecognized buildsystem for $1"
        exit 1
    fi
}

ARTIFACT=netconf-cli-$(git --git-dir ${TH_GIT_PATH}/.git rev-parse HEAD:submodules/).tar.xz

scp th-ci-logs@ci-logs.gerrit.cesnet.cz:artifacts/${TH_JOB_NAME}/${ARTIFACT} . \
    || true # ignore network errors

if [[ -f ${TH_JOB_WORKING_DIR}/${ARTIFACT} ]]; then
    tar -C ~/target -xvJf ${TH_JOB_WORKING_DIR}/${ARTIFACT}
else
    # rebuild everything from scratch

    CMAKE_OPTIONS="${CMAKE_OPTIONS} -DGEN_LANGUAGE_BINDINGS=ON -DGEN_PYTHON_BINDINGS=OFF -DGEN_JAVA_BINDINGS=OFF" emerge_dep libyang
    do_test_dep_cmake libyang -j${CI_PARALLEL_JOBS}

    emerge_dep libredblack --with-pic

    # sysrepo needs to use a persistent repo location
    CMAKE_OPTIONS="${CMAKE_OPTIONS} -DREPOSITORY_LOC=${PREFIX}/etc-sysrepo" emerge_dep sysrepo
    # These tests are only those which can run on the global repo.
    # They also happen to fail when run in parallel. That's expected, they manipulate a shared repository.
    do_test_dep_cmake sysrepo

    emerge_dep Catch
    do_test_dep_cmake Catch -j${CI_PARALLEL_JOBS}

    # Trompeloeil is a magic snowflake because it attempts to download and build Catch and kcov when building in a debug mode...
    CMAKE_BUILD_TYPE=Release emerge_dep trompeloeil

    emerge_dep docopt.cpp
    do_test_dep_cmake docopt.cpp -j${CI_PARALLEL_JOBS}

    emerge_dep spdlog
    do_test_dep_cmake spdlog -j${CI_PARALLEL_JOBS}

    # boost-spirit doesn't require installation
    pushd ${TH_GIT_PATH}/submodules/boost
    git submodule update --init
    ./bootstrap.sh --prefix=${PREFIX}
    ./b2 --ignore-site-config headers
    cp -LR boost ${PREFIX}/include/
    popd

    # examples are broked on clang+ubsan because of their STL override
    CMAKE_OPTIONS="${CMAKE_OPTIONS} -DBUILD_SHARED_LIBS=ON -DREPLXX_BuildExamples=OFF" emerge_dep replxx
    do_test_dep_cmake replxx -j${CI_PARALLEL_JOBS}

    tar -C ~/target -cvJf ${TH_JOB_WORKING_DIR}/${ARTIFACT} .
    ssh th-ci-logs@ci-logs.gerrit.cesnet.cz mkdir -p artifacts/${TH_JOB_NAME} \
        || true # ignore network errors
    rsync ${TH_JOB_WORKING_DIR}/${ARTIFACT} th-ci-logs@ci-logs.gerrit.cesnet.cz:artifacts/${TH_JOB_NAME}/ \
        || true # ignore network errors
fi

mkdir -p ${ZUUL_PROJECT}/build
cd ${ZUUL_PROJECT}/build
${CMAKE} -GNinja \
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Debug} \
    -DCMAKE_INSTALL_PREFIX=${PREFIX} \
    ${CMAKE_OPTIONS} \
    ${TH_GIT_PATH}
ninja-build

${CTEST} --output-on-failure
