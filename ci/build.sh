#!/bin/bash

set -eux -o pipefail
shopt -s failglob

ZUUL_JOB_NAME=$(jq < ~/zuul-env.json -r '.job')
ZUUL_PROJECT_SRC_DIR=$HOME/$(jq < ~/zuul-env.json -r '.project.src_dir')
ZUUL_PROJECT_SHORT_NAME=$(jq < ~/zuul-env.json -r '.project.short_name')

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

PREFIX=~/target
mkdir ${PREFIX}
BUILD_DIR=~/build
mkdir ${BUILD_DIR}
export PATH=${PREFIX}/bin:$PATH
export LD_LIBRARY_PATH=${PREFIX}/lib64:${PREFIX}/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
export PKG_CONFIG_PATH=${PREFIX}/lib64/pkgconfig:${PREFIX}/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}

ARTIFACT_URL=""

if [[ "$(jq < ~/zuul-env.json -r '.artifacts[0].project')" = "CzechLight/dependencies" &&
      "$(jq < ~/zuul-env.json -r '.artifacts[0].name')" = "tarball" ]]; then
    # FIXME: Zuul feeds us with wrong artifact (from a different provider) for some reason :(
    # so let's look into builds from the change pipeline and fetch their artifact directly
    #ARTIFACT_URL=$(jq < ~/zuul-env.json -r '.artifacts[0].url')

    CHANGE_BUILD_QUERY=$(jq -r '.items | map(select(.project.name == "CzechLight/dependencies"))[-1] | ("change=" + .change + "&patchset=" + .patchset)' < ~/zuul-env.json)
    if [[ "${CHANGE_BUILD_QUERY}" != "null" ]]; then
        # We depend on some change from project CzechLight/dependencies, let's look at the latest one
        ZUUL_TENANT=$(jq < ~/zuul-env.json -r '.tenant')
        ARTIFACT_URL=$(curl "https://zuul.gerrit.cesnet.cz/api/tenant/${ZUUL_TENANT}/builds?pipeline=check&job_name=${ZUUL_JOB_NAME}&${CHANGE_BUILD_QUERY}" | jq -r '.[].artifacts[0].url')
    fi
fi

DEP_SUBMODULE_COMMIT=$(git ls-tree -l master submodules/dependencies | cut -d ' ' -f 3)

if [[ -z "${ARTIFACT_URL}" ]]; then
    # fallback to a promoted artifact
    ARTIFACT_URL=https://ci-logs.gerrit.cesnet.cz/t/public/artifacts/${ZUUL_JOB_NAME}/czechlight-dependencies-${DEP_SUBMODULE_COMMIT}.tar.xz
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
mv html ~/zuul-output/docs/
