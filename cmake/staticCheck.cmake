find_package(Python3 COMPONENTS Interpreter) # sets Python3_EXECUTABLE
find_program(CLANG_FORMAT_COMMAND clang-format)

function(AddStaticCheckTarget)
    set(__options
        )
    set(__one_val_required
        TARGET_NAME
        SOURCE_DIR
        )
    set(__one_val_optional)
    set(__multi_val)
    cmake_parse_arguments(ARG "${__options}" "${__one_val_required}" "${__multi_val}" ${ARGN})

    if (NOT TARGET ${ARG_TARGET_NAME})
        message(SEND_ERROR "${ARG_TARGET_NAME} is not a target")
        return()
    endif()

    if ((NOT CLANG_FORMAT_COMMAND) OR (NOT Python3_EXECUTABLE))
        return()
    endif()

    file(GLOB_RECURSE allFiles "${ARG_SOURCE_DIR}/[^.]*" )
    set(globHeaders ${allFiles})
    list(FILTER globHeaders INCLUDE REGEX "\\.hpp$")
    set(globSources ${allFiles})
    list(FILTER globSources INCLUDE REGEX "\\.cpp$")

    set(sources ${globSources} ${globHeaders})
    if (NOT sources)
        message(SEND_ERROR "No sources defined for ${ARG_TARGET_NAME}, remove the target creation")
        return()
    endif()

    set(pythonScriptsRoot ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

    set(outCheckFiles)
    foreach(file ${sources})
        string(REPLACE "${ARG_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/StaticCheck${ARG_TARGET_NAME}" outFile "${file}")
        set(outCheckFile ${outFile}.check)
        set(checkCommands)
        list(APPEND checkCommands
            COMMAND ${Python3_EXECUTABLE} ${pythonScriptsRoot}/clangFormat.py -i ${file} -f ${CLANG_FORMAT_COMMAND}
            )

        add_custom_command(
            OUTPUT ${outCheckFile}
            ${checkCommands}
            COMMAND ${CMAKE_COMMAND} -E touch ${outCheckFile}
            DEPENDS ${file}
            )
        list(APPEND outCheckFiles ${outCheckFile})
    endforeach()

    set(name _StaticCheck_${ARG_TARGET_NAME}${ARG_SUFFIX})
    add_custom_target(${name} DEPENDS ${outCheckFiles})
    add_dependencies(${ARG_TARGET_NAME} ${name})
endfunction()
