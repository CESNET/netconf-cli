if(GIT_EXECUTABLE AND EXISTS ${PROJECT_SOURCE_DIR}/.git)
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --dirty --long --always --tags
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE ${VERSION_VARIABLE})
    string(REPLACE "\n" "" "${VERSION_VARIABLE}" ${${VERSION_VARIABLE}})
endif()

if(${VERSION_VARIABLE})
    set(${VERSION_VARIABLE}_H
        "#define ${VERSION_VARIABLE} \"${${VERSION_VARIABLE}}\"\n"
        )
    message(STATUS "Building ${PROJECT_NAME} Git version ${${VERSION_VARIABLE}}")
else()
    set(${VERSION_VARIABLE}_H
        "#define ${VERSION_VARIABLE} \"${FALLBACK_VERSION}\"\n"
        )
    message(STATUS "Building ${PROJECT_NAME} version: ${${VERSION_VARIABLE}}")
endif()

file(WRITE ${PROJECT_BINARY_DIR}/${VERSION_VARIABLE}.h.in ${${VERSION_VARIABLE}_H})
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_BINARY_DIR}/${VERSION_VARIABLE}.h.in ${PROJECT_BINARY_DIR}/${VERSION_VARIABLE}.h)
