#!/bin/bash

set -eux -o pipefail
shopt -s failglob

ZUUL_JOB_NAME=$(jq < ~/zuul-env.json -r '.job')
ZUUL_TENANT=$(jq < ~/zuul-env.json -r '.tenant')
ZUUL_PROJECT_SRC_DIR=$HOME/$(jq < ~/zuul-env.json -r '.project.src_dir')
ZUUL_PROJECT_SHORT_NAME=$(jq < ~/zuul-env.json -r '.project.short_name')
ZUUL_GERRIT_HOSTNAME=$(jq < ~/zuul-env.json -r '.project.canonical_hostname')

CI_PARALLEL_JOBS=$(grep -c '^processor' /proc/cpuinfo)
CMAKE_OPTIONS=""
CFLAGS=""
CXXFLAGS=""
LDFLAGS=""

if [[ $ZUUL_JOB_NAME =~ .*-clang.* ]]; then
    export CC=clang
    export CXX=clang++
    export LD=clang
    export CXXFLAGS="-stdlib=libc++"
fi

if [[ $ZUUL_JOB_NAME =~ .*-ubsan ]]; then
    export CFLAGS="-fsanitize=undefined ${CFLAGS}"
    export CXXFLAGS="-fsanitize=undefined ${CXXFLAGS}"
    export LDFLAGS="-fsanitize=undefined ${LDFLAGS}"
fi

if [[ $ZUUL_JOB_NAME =~ .*-asan ]]; then
    export CFLAGS="-fsanitize=address ${CFLAGS}"
    export CXXFLAGS="-fsanitize=address ${CXXFLAGS}"
    export LDFLAGS="-fsanitize=address ${LDFLAGS}"
fi

if [[ $ZUUL_JOB_NAME =~ .*-tsan ]]; then
    export CFLAGS="-fsanitize=thread ${CFLAGS}"
    export CXXFLAGS="-fsanitize=thread ${CXXFLAGS}"
    export LDFLAGS="-fsanitize=thread ${LDFLAGS}"
fi

PREFIX=~/target
mkdir ${PREFIX}
BUILD_DIR=~/build
mkdir ${BUILD_DIR}
export PATH=${PREFIX}/bin:$PATH
export LD_LIBRARY_PATH=${PREFIX}/lib64:${PREFIX}/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
export PKG_CONFIG_PATH=${PREFIX}/lib64/pkgconfig:${PREFIX}/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}

ARTIFACT_URL=""

if [[ "$(jq < ~/zuul-env.json -r '.artifacts[-1].project')" = "CzechLight/dependencies" &&
      "$(jq < ~/zuul-env.json -r '.artifacts[-1].name')" = "tarball" ]]; then
    ARTIFACT_URL=$(jq < ~/zuul-env.json -r '.artifacts[-1].url')
fi

DEP_SUBMODULE_COMMIT=$(git ls-tree -l master submodules/dependencies | cut -d ' ' -f 3)

if [[ -z "${ARTIFACT_URL}" ]]; then
    # fallback to a promoted artifact
    ARTIFACT_URL="https://object-store.cloud.muni.cz/swift/v1/ci-artifacts-${ZUUL_TENANT}/${ZUUL_GERRIT_HOSTNAME}/CzechLight/dependencies/${ZUUL_JOB_NAME}/${DEP_SUBMODULE_COMMIT}.tar.xz"
fi

ARTIFACT_FILE=$(basename ${ARTIFACT_URL})
DEP_HASH_FROM_ARTIFACT=$(echo "${ARTIFACT_FILE}" | sed -e 's/^czechlight-dependencies-//' -e 's/\.tar\.xz$//')
if [[ "${DEP_HASH_FROM_ARTIFACT}" != "${DEP_SUBMODULE_COMMIT}" ]]; then
    echo "Mismatched artifact: HEAD of ./submodules/dependencies does not match artifact commit ref"
    exit 1
fi
curl ${ARTIFACT_URL} --output ${ARTIFACT_FILE}
tar -C ${PREFIX} -xf ${ARTIFACT_FILE}
rm ${ARTIFACT_FILE}

cd ${BUILD_DIR}
cmake -GNinja -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Debug} -DCMAKE_INSTALL_PREFIX=${PREFIX} ${CMAKE_OPTIONS} ${ZUUL_PROJECT_SRC_DIR}
ninja-build
ctest -j${CI_PARALLEL_JOBS} --output-on-failure
ninja-build doc
pushd html
zip -r ~/zuul-output/docs/html.zip .
popd
