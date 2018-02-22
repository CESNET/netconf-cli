#!/bin/bash
BUILD_TIMEOUT=30m
timeout -k $BUILD_TIMEOUT $BUILD_TIMEOUT $TH_GIT_PATH/ci/do-build.sh
