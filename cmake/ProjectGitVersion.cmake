find_package(Git REQUIRED)

macro(prepare_git_version VERSION_VARIABLE FALLBACK_VERSION)
    set(${VERSION_VARIABLE}-version_files ${PROJECT_BINARY_DIR}/${VERSION_VARIABLE}.h)
    add_custom_target(target-${VERSION_VARIABLE} DEPENDS build_version_fake_file-${VERSION_VARIABLE})
    add_custom_command(OUTPUT build_version_fake_file-${VERSION_VARIABLE} ${${VERSION_VARIABLE}-version_files}
        COMMAND ${CMAKE_COMMAND} -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
            -DVERSION_VARIABLE=${VERSION_VARIABLE}
            -DFALLBACK_VERSION=${FALLBACK_VERSION}
            -DPROJECT_NAME=${PROJECT_NAME}
            -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
            -DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}
            -P ${PROJECT_SOURCE_DIR}/cmake/ProjectGitVersionRunner.cmake)
    set_source_files_properties(${PROJECT_BINARY_DIR}/${${VERSION_VARIABLE}.h}
        PROPERTIES
        GENERATED TRUE
        HEADER_FILE_ONLY TRUE)
endmacro()
