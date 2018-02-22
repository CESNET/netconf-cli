#!/bin/bash

set -eux -o pipefail
shopt -s failglob

# We're reusing our artifacts, so we absolutely need a stable destdir.
# Turbo-hipster takes care of cleaning up the mess betweeb builds.
PREFIX=~/target
mkdir ${PREFIX}
export PATH=${PREFIX}/bin:$PATH
export LD_LIBRARY_PATH=${PREFIX}/lib64:${PREFIX}/lib
export PKG_CONFIG_PATH=${PREFIX}/lib64/pkgconfig:${PREFIX}/lib/pkgconfig

if [[ $TH_JOB_NAME =~ .*-sanitizers-.* ]]; then
    # https://gitlab.kitware.com/cmake/cmake/issues/16609
    CMAKE_OPTIONS="${CMAKE_OPTIONS} -DTHREADS_HAVE_PTHREAD_ARG:BOOL=ON"
fi

# force-enable tests for packages which use, eh, interesting setup
# - libyang and libnetconf2 copmare CMAKE_BUILD_TYPE to lowercase "debug"...
CMAKE_OPTIONS="${CMAKE_OPTIONS} -DENABLE_BUILD_TESTS=ON -DENABLE_VALGRIND_TESTS=OFF"

mkdir -p ${ZUUL_PROJECT}/build
cd ${ZUUL_PROJECT}/build
${CMAKE} -GNinja \
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Debug} \
    -DCMAKE_INSTALL_PREFIX=${PREFIX} \
    ${CMAKE_OPTIONS} \
    ${TH_GIT_PATH}
ninja-build

${CTEST} --output-on-failure
