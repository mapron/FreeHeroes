# Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
# SPDX-License-Identifier: MIT
# See LICENSE file for details.

set(FH_PREFIX FH)

include(GenerateExportHeader)
include(CheckCXXCompilerFlag)
include(staticCheck)

function(AddCompilerFlagIfSupported)
    foreach(instruction ${ARGN})
        CHECK_CXX_COMPILER_FLAG(${instruction} exist_${instruction})
        if(${exist_${instruction}})
            add_compile_options(${instruction})
        endif()
    endforeach()
endfunction()

# qt5_make_output_file saved from deprecated
# macro used to create the names of output files preserving relative dirs
macro(qt5_make_output_file_freeheroes infile prefix ext outfile )
    string(LENGTH ${CMAKE_CURRENT_BINARY_DIR} _binlength)
    string(LENGTH ${infile} _infileLength)
    set(_checkinfile ${CMAKE_CURRENT_SOURCE_DIR})
    if(_infileLength GREATER _binlength)
        string(SUBSTRING "${infile}" 0 ${_binlength} _checkinfile)
        if(_checkinfile STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
            file(RELATIVE_PATH rel ${CMAKE_CURRENT_BINARY_DIR} ${infile})
        else()
            file(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR} ${infile})
        endif()
    else()
        file(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR} ${infile})
    endif()
    if(WIN32 AND rel MATCHES "^([a-zA-Z]):(.*)$") # absolute path
        set(rel "${CMAKE_MATCH_1}_${CMAKE_MATCH_2}")
    endif()
    set(_outfile "${CMAKE_CURRENT_BINARY_DIR}/${rel}")
    string(REPLACE ".." "__" _outfile ${_outfile})
    get_filename_component(outpath ${_outfile} PATH)
    if(CMAKE_VERSION VERSION_LESS "3.14")
        get_filename_component(_outfile_ext ${_outfile} EXT)
        get_filename_component(_outfile_ext ${_outfile_ext} NAME_WE)
        get_filename_component(_outfile ${_outfile} NAME_WE)
        string(APPEND _outfile ${_outfile_ext})
    else()
        get_filename_component(_outfile ${_outfile} NAME_WLE)
    endif()
    file(MAKE_DIRECTORY ${outpath})
    set(${outfile} ${outpath}/${prefix}${_outfile}.${ext})
endmacro()

function(CreateUiRules outfiles extraIncludes)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs OPTIONS)

    cmake_parse_arguments(_WRAP_UI "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(ui_files ${_WRAP_UI_UNPARSED_ARGUMENTS})
    set(ui_options ${_WRAP_UI_OPTIONS})
    set(includes)

    foreach(it ${ui_files})
        get_filename_component(outfile ${it} NAME_WE)
        get_filename_component(infile ${it} ABSOLUTE)

        qt5_make_output_file_freeheroes(${it} ui_ h outfile)

        get_filename_component(incl_path ${outfile} PATH)

#[[
 What's the problem with textReplace after ui generated?
 Well, Qt for some reason do not generate parent pass for widgets adding into stack widget,
 as they will be set their parent later, probably.
 We highly rely  on every parent passed in constructor is valid in any time, so here we have dirty hack:
 Add explicit parent pass to every thing "Widget" with empty parenthesis.

 Another way around is probably avoid stackedWidget at all..
#]]

        list(APPEND includes ${incl_path} )
        add_custom_command(OUTPUT ${outfile}
          COMMAND ${Qt5Widgets_UIC_EXECUTABLE}
          ARGS ${ui_options} -o ${outfile} ${infile}
        #  COMMAND ${CMAKE_COMMAND}
       #   ARGS -DFILENAME=${outfile}  -P  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/textReplace.cmake
          MAIN_DEPENDENCY ${infile} VERBATIM)
        list(APPEND ${outfiles} ${outfile})
    endforeach()
    list(REMOVE_DUPLICATES includes)
    set(${outfiles} ${${outfiles}} PARENT_SCOPE)
    set(${extraIncludes} ${includes} PARENT_SCOPE)
endfunction()

# modified QT5_WRAP_CPP
# pathed qt macro to force always have output cpp file. However, it now does not require to run cmake to detect Q_OBJECT macro.
function(CreateMocRules outfiles includes defines)
    string(REPLACE ";" "," includesMoc "${${includes}}")
    string(REPLACE ";" "," definesMoc "${${defines}}")
    foreach(it ${ARGN})
        get_filename_component(it ${it} ABSOLUTE)
        qt5_make_output_file_freeheroes(${it} moc_ cpp outfile)
        add_custom_command(
            OUTPUT ${outfile}
            COMMAND ${CMAKE_COMMAND} -DINFILE=${it} -DOUTFILE=${outfile} -DINCLUDES=${includesMoc} -DDEFINES=${definesMoc} -P ${CMAKE_BINARY_DIR}/mocWrapper.cmake
            DEPENDS ${it}
            VERBATIM)
        list(APPEND ${outfiles} ${outfile})
    endforeach()
    set(${outfiles} ${${outfiles}} PARENT_SCOPE)
endfunction()

# filename - outfile
# fileListName - variable name with list
# basedir - root for replace
# qrcPrefix - prefix for configuration
function(GenerateQrc filename fileListName basedir qrcPrefix)

    foreach (file ${${fileListName}})
        string(REPLACE "${basedir}/" "" file "${file}")
        set(QRC_FILES "${QRC_FILES}<file>${file}</file>\n")
    endforeach()
    configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cmake/qrcTemplate.qrc.in ${filename} @ONLY )
endfunction()

function(AddQrcOutput rccName qrcName)
    set(fileListAbsConfig ${ARGN})
    configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cmake/qrcTemplate.qrc.in ${qrcName} @ONLY )
    source_group("Qt Resource Files" FILES ${qrcName})
    add_custom_command(OUTPUT ${rccName}
                       COMMAND ${Qt5Core_RCC_EXECUTABLE}
                       ARGS --no-compress --binary -o ${rccName} ${qrcName}
                       MAIN_DEPENDENCY ${qrcName}
                       DEPENDS ${fileListAbsConfig} VERBATIM)
endfunction()

function(GenerateQrcFromAssets resourceFolder )
    set(srcDir ${CMAKE_CURRENT_SOURCE_DIR}/guiAssets/${resourceFolder})
    set(destDir ${CMAKE_BINARY_DIR}/assetsCompiled/${resourceFolder})
    set(qrcName ${CMAKE_BINARY_DIR}/assetsCompiled/${resourceFolder}.qrc)
    set(rccName ${CMAKE_BINARY_DIR}/assetsCompiled/${resourceFolder}.rcc)
    file(MAKE_DIRECTORY "${destDir}")

    set(QRC_PREFIX ${resourceFolder}) # name is important for .in file
    set(QRC_FILES)                    # name is important for .in file

    set(masks ${srcDir}/*.png)
    file(GLOB_RECURSE fileListAbsSrc   ${masks})
    set(fileListAbsConfig)
    foreach(absFile ${fileListAbsSrc})
        string(REPLACE "${srcDir}/" "" relFile "${absFile}")
        set(destFileAbs "${destDir}/${relFile}")
        list(APPEND fileListAbsConfig ${destFileAbs})
        configure_file(${absFile} ${destFileAbs} COPYONLY)
        string(REPLACE "${resourceFolder}/" "" file "${relFile}")
        set(QRC_FILES "${QRC_FILES}<file alias=\"${file}\">${resourceFolder}/${relFile}</file>\n")
    endforeach()
    AddQrcOutput(${rccName} ${qrcName} ${fileListAbsConfig})
endfunction()

function(GenerateQrcWithTranslations resourceFolder translationsRoot)
    set(srcDir ${translationsRoot})
    set(destDir ${CMAKE_BINARY_DIR}/assetsCompiled/${resourceFolder})
    set(qrcName ${CMAKE_BINARY_DIR}/assetsCompiled/${resourceFolder}.qrc)
    set(rccName ${CMAKE_BINARY_DIR}/assetsCompiled/${resourceFolder}.rcc)
    file(MAKE_DIRECTORY "${destDir}")
    set(masks ${srcDir}/*.ts)
    file(GLOB_RECURSE fileListAbsSrc   ${masks})

    set(QRC_PREFIX ${resourceFolder}) # name is important for .in file
    set(QRC_FILES)                    # name is important for .in file

    set(fileListAbsConfig)
    foreach(tsFileAbs ${fileListAbsSrc})
        string(REPLACE "${srcDir}/" "" relFile "${tsFileAbs}")
        string(REPLACE ".ts" ".qm" relFile "${relFile}")
        set(qmFileAbs "${destDir}/${relFile}")
        list(APPEND fileListAbsConfig ${qmFileAbs})

        #configure_file(${absFile} ${destFileAbs} COPYONLY)
        add_custom_command(OUTPUT ${qmFileAbs}
                           COMMAND ${Qt5_LRELEASE_EXECUTABLE}
                           ARGS ${tsFileAbs} -qm ${qmFileAbs}
                           MAIN_DEPENDENCY ${tsFileAbs} VERBATIM)

        string(REPLACE "${resourceFolder}/" "" file "${relFile}")
        set(QRC_FILES "${QRC_FILES}<file alias=\"${file}\">${resourceFolder}/${relFile}</file>\n")
    endforeach()
    AddQrcOutput(${rccName} ${qrcName} ${fileListAbsConfig})
endfunction()

function(AddResourceCustomTarget name)
    set(resourceIds ${ARGN})
    set(rccList)
    foreach(id ${resourceIds})
        list(APPEND rccList ${CMAKE_BINARY_DIR}/assetsCompiled/${id}.rcc)
    endforeach()
    add_custom_target(${name} ALL DEPENDS ${rccList})
    install(FILES ${rccList} DESTINATION bin/assetsCompiled)
endfunction()

# function for target declaration.
function(AddTarget)
    set(options FH NO_DEFAULT_GLOB QT)
    set(oneValueArgs NAME OUTPUT_NAME ROOT TYPE MAIN_INCLUDE)
    set(multiValueArgs SRC INCLUDES DEPS DEPS_FH OPTIONS DEFINES EXCLUDE_FILES MOC_INCLUDES MOC_DEFINES)
    cmake_parse_arguments(AddTarget "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
    set(sources)
    set(headers)
    set(subdir ${CMAKE_CURRENT_SOURCE_DIR}/)
    if (AddTarget_ROOT)
        set(subdir ${CMAKE_CURRENT_SOURCE_DIR}/${AddTarget_ROOT}/)
    endif()
    set(masksCpp *.cpp *.c ${AddTarget_SRC})
    set(masksHeaders *.hpp *.h)
    if (AddTarget_NO_DEFAULT_GLOB)
        set(masksCpp ${AddTarget_SRC})
        set(masksHeaders)
    endif()
    foreach (maskl ${masksCpp} )
        file(GLOB src ${subdir}${maskl})
        list(APPEND sources ${src})
    endforeach()
    foreach (maskl ${masksHeaders} )
        file(GLOB src ${subdir}${maskl})
        list(APPEND headers ${src})
    endforeach()
    foreach (mask ${AddTarget_EXCLUDE_FILES})
        list(FILTER sources EXCLUDE REGEX ${mask})
    endforeach()
    if (AddTarget_QT)
        file(GLOB uiFiles ${subdir}*.ui)
        source_group("Form Files" FILES ${uiFiles})

        CreateUiRules(generatedUiFiles uiIncludes ${uiFiles})
        list(APPEND AddTarget_INCLUDES ${uiIncludes})

        list(APPEND sources ${uiFiles} )
        set(mocIncludes ${subdir} ${AddTarget_MOC_INCLUDES})
        set(mocDefines ${AddTarget_DEFINES} ${AddTarget_MOC_DEFINES})
        CreateMocRules(sources "mocIncludes" "mocDefines" ${headers} )
    endif()
    set(name ${AddTarget_NAME})
    if (AddTarget_FH)
        set(name ${FH_PREFIX}${name})
    endif()
    if ((AddTarget_TYPE STREQUAL "app") OR (AddTarget_TYPE STREQUAL "console_app"))
        if (WIN32 AND NOT (CMAKE_BUILD_TYPE STREQUAL "Debug") AND NOT (AddTarget_TYPE STREQUAL "console_app"))
            set(w32 "WIN32")
        endif()
        add_executable(${name} ${w32} ${sources} ${headers})
    else()
        if (AddTarget_TYPE STREQUAL "static")
            add_library(${name} STATIC ${sources} ${headers})
        else()
            add_library(${name} SHARED ${sources} ${headers})
            generate_export_header(${name} BASE_NAME ${AddTarget_NAME} EXPORT_FILE_NAME ${AddTarget_NAME}Export.hpp )
        endif()
    endif()
    if (AddTarget_OUTPUT_NAME)
        set_target_properties(${name} PROPERTIES OUTPUT_NAME ${AddTarget_OUTPUT_NAME})
    elseif((AddTarget_TYPE STREQUAL "app"))
        set_target_properties(${name} PROPERTIES OUTPUT_NAME ${AddTarget_NAME})
    endif()
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    foreach (inc ${AddTarget_INCLUDES})
        target_include_directories(${name} PRIVATE ${inc})
    endforeach()
    if (AddTarget_MAIN_INCLUDE)
        target_include_directories(${name} PUBLIC ${AddTarget_MAIN_INCLUDE})
    else ()
        target_include_directories(${name} PUBLIC ${subdir})
    endif()
    foreach (dep ${AddTarget_DEPS})
        target_link_libraries(${name} PRIVATE ${dep})
    endforeach()
    foreach (dep ${AddTarget_DEPS_FH})
        target_link_libraries(${name} PRIVATE ${FH_PREFIX}${dep})
    endforeach()
    foreach (opt ${AddTarget_OPTIONS})
        target_compile_options(${name} PRIVATE ${opt})
    endforeach()
    foreach (opt ${AddTarget_DEFINES})
        target_compile_definitions(${name} PRIVATE ${opt})
    endforeach()
    if (AddTarget_FH)
        set_property(TARGET ${name} PROPERTY FOLDER ${subdir})
    endif()
    # source_group("SRC" FILES "${sources}")
    if (AddTarget_QT)
        set_target_properties(${name} PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
    endif()
    if (AddTarget_TYPE STREQUAL "app")
        install(TARGETS ${name}
                RUNTIME DESTINATION bin
                )
    elseif(AddTarget_TYPE STREQUAL "shared")
        install(TARGETS ${name}
                        RUNTIME DESTINATION bin
                        )
    endif()

    if (AddTarget_FH)
        AddStaticCheckTarget(TARGET_NAME ${name} SOURCE_DIR ${subdir})
    endif()
endfunction()

function(AddTargetInterface)
    set(options FH)
    set(oneValueArgs NAME)
    set(multiValueArgs INCLUDES DEFINES DEPS DEPS_FH)
    cmake_parse_arguments(AddTarget "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
    set(name ${AddTarget_NAME})
    if (AddTarget_FH)
        set(name ${FH_PREFIX}${name})
    endif()
    add_library(${name} INTERFACE)
    foreach (inc ${AddTarget_INCLUDES})
        target_include_directories(${name} INTERFACE ${inc})
    endforeach()
    foreach (opt ${AddTarget_DEFINES})
        target_compile_definitions(${name} INTERFACE ${opt})
    endforeach()
    foreach (dep ${AddTarget_DEPS})
        target_link_libraries(${name} INTERFACE ${dep})
    endforeach()
    foreach (dep ${AddTarget_DEPS_FH})
        target_link_libraries(${name} INTERFACE ${FH_PREFIX}${dep})
    endforeach()
endfunction()

function(AddTargetHeaderOnly)
    set(options FH)
    set(oneValueArgs NAME)
    set(multiValueArgs INCLUDES DEFINES DEPS DEPS_FH)
    cmake_parse_arguments(AddTarget "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
    set(name ${AddTarget_NAME})
    if (AddTarget_FH)
        set(name ${FH_PREFIX}${name})
    endif()
    set(headerList)
    foreach (inc ${AddTarget_INCLUDES})
        file(GLOB src ${inc}/*.hpp)
        set(headerList ${headerList} ${src})
    endforeach()
    set(includeList)
    foreach (hdr ${headerList})
        set(includeList "${includeList} #include \"${hdr}\"\r\n")
    endforeach()
    set(stubName ${CMAKE_CURRENT_BINARY_DIR}/headerOnlyStub${name}.cpp)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/headerOnlyStub.cpp.in
        ${stubName})
    set_source_files_properties(${stubName} PROPERTIES GENERATED TRUE)
    add_library(${name} STATIC ${stubName} ${headerList})
    foreach (inc ${AddTarget_INCLUDES})
        target_include_directories(${name} INTERFACE ${inc})
    endforeach()
    foreach (opt ${AddTarget_DEFINES})
        target_compile_definitions(${name} INTERFACE ${opt})
    endforeach()
    foreach (dep ${AddTarget_DEPS})
        target_link_libraries(${name} PUBLIC ${dep})
    endforeach()
    foreach (dep ${AddTarget_DEPS_FH})
        target_link_libraries(${name} PUBLIC ${FH_PREFIX}${dep})
    endforeach()
    AddStaticCheckTarget(TARGET_NAME ${name} SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${AddTarget_INCLUDES})
endfunction()
