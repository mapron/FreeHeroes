include_guard(GLOBAL)

function(AddCompilerFlagIfSupported)
    foreach(instruction ${ARGN})
        CHECK_CXX_COMPILER_FLAG(${instruction} exist_${instruction})
        if(${exist_${instruction}})
            add_compile_options(${instruction})
        endif()
    endforeach()
endfunction()

# Expands conditions in ListName using brackets (brackets must be separated):
#[[
set(test
    [ WIN32 win-only0 win-only1 ]
    [ APPLE apple-only0 ]
    [ NOT APPLE not-apple0 ]
    [ WIN32 ? win-only2 ] # if-then
    [ WIN32 ? win-only4 : not-win0 ] # if-then-else
    )
ExpandListConditions(test)
#]]
function(ExpandListConditions ListName)
    set(resultElements)
    foreach (element ${${ListName}})
        if (NOT ("${element}" MATCHES "\\[;.*"))
            list(APPEND resultElements ${element})
            continue()
        endif()
        if ("${element}" STREQUAL "[;]")
            message(SEND_ERROR "Empty square brackets conditions not allowed!")
            continue()
        endif()
        # remove opening and closing brackets
        string(REPLACE "[;" "" element "${element}")
        string(REPLACE ";]" "" element "${element}")

        string(FIND "${element}" ";?" thenIndex)
        if (NOT thenIndex EQUAL -1) # ternary

            string(FIND "${element}" ";:" elseIndex)
            string(LENGTH "${element}" len)
            string(SUBSTRING "${element}" 0 ${thenIndex} condition)

            set(elseValue)
            if (NOT elseIndex EQUAL -1)
                if (elseIndex LESS thenIndex)
                    message(FATAL_ERROR "Wrong [ condition ? then : else ] syntax: expects ':' after '?':\n${element}")
                endif()
                math(EXPR elseBeginIndex "${elseIndex} + 2" )
                math(EXPR elseLen "${len} - ${elseBeginIndex}" )
                string(SUBSTRING "${element}" ${elseBeginIndex} ${elseLen} elseValue)
            else()
                set(elseIndex ${len})
            endif()

            if (NOT elseIndex EQUAL thenIndex)
                math(EXPR thenBeginIndex "${thenIndex} + 2" )
                math(EXPR thenLen "${elseIndex} - ${thenBeginIndex}" )
                string(SUBSTRING "${element}" ${thenBeginIndex} ${thenLen} thenValue)

                if (${condition})
                    list(APPEND resultElements ${thenValue})
                else()
                    list(APPEND resultElements ${elseValue})
                endif()
            endif()

        else () # simple
            list(LENGTH element len)
            list(GET element 0 condition)
            list(REMOVE_AT element 0)
            set(conditionResult false)
            set(inverse false)
            if (condition STREQUAL "NOT")
                set(inverse true)
                list(GET element 0 condition)
                list(REMOVE_AT element 0)
            endif()
            if (${condition})
                set(conditionResult true)
            endif()
            # conditionResult = !conditionResult;
            if (inverse)
                if (conditionResult)
                    set(conditionResult false)
                else()
                    set(conditionResult true)
                endif()
            endif()
            if (conditionResult AND element)
                list(APPEND resultElements ${element})
            endif()
        endif()
    endforeach()
    set(${ListName} ${resultElements} PARENT_SCOPE)
endfunction()

# better replacement for cmake_parse_arguments().
macro(ParseArgumentsWithConditions argName options oneValRequired oneValOptional multiVal)
    set(__allArgs ${ARGN})
    ExpandListConditions(__allArgs)
    cmake_parse_arguments(${argName} "${options}" "${oneValRequired};${oneValOptional}" "${multiVal}" ${__allArgs})
    if(${argName}_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "invalid arguments: ${__allArgs}, unparsed:${${argName}_UNPARSED_ARGUMENTS}")
    endif()
    foreach(__varName ${oneValRequired})
        if(NOT DEFINED ${argName}_${__varName})
            message(FATAL_ERROR "Required option is not defined '${__varName}'")
        endif()
    endforeach()
endmacro()

# filter platform-specific files out.
function(filterSources listToFilter)
    set(regexList ${ARGN})
    if( NOT WIN32 )
        list(APPEND regexList "(/[Ww]in/|_[Ww]in[.])")
    endif()
    if(NOT APPLE)
        list(APPEND regexList "(/[Mm]ac/|_[Mm]ac[.])")
    endif()
    if(NOT LINUX)
        list(APPEND regexList "(/[Ll]inux/|_[Ll]inux[.])")
    endif()
    if(NOT UNIX)
        list(APPEND regexList "(/[Uu]nix/|_[Uu]nix[.])")
    endif()

    string(REPLACE ";" "|" regexList "(${regexList})")
    list(FILTER ${listToFilter} EXCLUDE REGEX "${regexList}")

    set( ${listToFilter} ${${listToFilter}} PARENT_SCOPE)
endfunction()
