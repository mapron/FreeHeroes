# Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
# SPDX-License-Identifier: MIT
# See LICENSE file for details.
include_guard(GLOBAL)

include(GenerateExportHeader)
include(CheckCXXCompilerFlag)
include(staticCheck)
include(utils)
include(qt_helpers)

function(AddResourceCustomTarget name)
    set(resourceIds ${ARGN})
    set(rccList)
    foreach(id ${resourceIds})
        list(APPEND rccList ${CMAKE_BINARY_DIR}/assetsCompiled/${id}.rcc)
    endforeach()
    add_custom_target(${name} ALL DEPENDS ${rccList})
    install(FILES ${rccList} DESTINATION bin/assetsCompiled)
endfunction()

function(AddInstallArchiveTarget name)
    set(archive ${CMAKE_BINARY_DIR}/InstallData.7z)
    set(compressionFlags -mx=9 -myx=9)
    if(fastCompression)
        set(compressionFlags -mx=1 -myx=0)
    endif()
    find_program(7zip 7z.exe PATHS "C:/Program Files/7-Zip" "C:/Program Files (x86)/7-Zip" REQUIRED)
    add_custom_command(OUTPUT "${archive}"
        COMMAND ${CMAKE_COMMAND} -E remove "${archive}"
        COMMAND "${7zip}" a ${compressionFlags} "${archive}" *
        WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin
        COMMENT "Archiving ${archive}"
        DEPENDS install
        )
    add_custom_target(${name} DEPENDS ${archive})
endfunction()

function(MakeTargetExport name)
    string(TOUPPER "${name}_EXPORT" DEFINITION_NAME)
    set(TARGET ${name})
    set(headerName  ${CMAKE_CURRENT_BINARY_DIR}/export/${name}Export.hpp)
    configure_file(${CMAKE_SOURCE_DIR}/cmake/ExportLib.h.in ${headerName} @ONLY)
    target_sources(${name} PRIVATE ${headerName})
endfunction()

# type = shared|static|interface|app_ui|app_console|app_bundle|header_only
function(MakeTarget name type excludeAll)
    if (excludeAll)
        set(excludeAll EXCLUDE_FROM_ALL)
    else()
        set(excludeAll)
    endif()
    set( CreateTarget )
    if(type STREQUAL static)
        add_library(${name} STATIC ${excludeAll})
        MakeTargetExport(${name})
    elseif(type STREQUAL shared)
        add_library(${name} SHARED ${excludeAll})
        MakeTargetExport(${name})
    elseif(type STREQUAL interface)
        add_library(${name} INTERFACE)
    elseif(type STREQUAL app_ui)
        if (WIN32)
            add_executable(${name} WIN32 ${excludeAll})
        else()
            add_executable(${name} ${excludeAll})
        endif()
    elseif(type STREQUAL app_console)
        add_executable(${name} ${excludeAll})
    elseif(type STREQUAL app_bundle)
        if (APPLE)
            add_executable(${name} MACOSX_BUNDLE ${excludeAll})
        elseif(WIN32)
            add_executable(${name} WIN32 ${excludeAll})
        else()
            add_executable(${name} ${excludeAll})
        endif()
    elseif(type STREQUAL header_only)
        add_library(${name} STATIC ${excludeAll})
    else()
        message( FATAL_ERROR "Unknown target type: ${type}" )
    endif()
endfunction()

function(MakeTargetSources name includes sourceDir useQt generateStub extraSources excludeSources extraPostprocess)
    file(GLOB_RECURSE globbedFiles "${sourceDir}/[^.]*" )
    list(APPEND globbedFiles ${extraSources})
    filterSources(globbedFiles ${excludeSources})
    set(headers ${globbedFiles})
    set(sources ${globbedFiles})
    list(FILTER headers INCLUDE REGEX "\\.(h|hpp)$")
    list(FILTER sources INCLUDE REGEX "\\.(c|cpp|cxx|mm|m)$")

    if(useQt)
        set(forms ${globbedFiles})
        set(resources ${globbedFiles})
        list(FILTER forms INCLUDE REGEX "\\.ui$")
        list(FILTER resources INCLUDE REGEX "\\.qrc$")
        
        if (forms)
            CreateUiRules(generatedUiFiles uiIncludes "${extraPostprocess}" ${forms})
            if (uiIncludes)
                list(APPEND sources ${generatedUiFiles})
                target_include_directories(${name} PRIVATE ${uiIncludes})
            endif()
        endif()

        set(mocIncludes ${includes} ${sourceDir})
        set(mocDefines)
        CreateMocRules(sources "mocIncludes" "mocDefines" ${headers} )
    endif()
    if (generateStub)
        set(stubName ${CMAKE_CURRENT_BINARY_DIR}/stubs/Stub${name}.cpp)
        set(includeList)
        foreach (hdr ${headers})
            set(includeList "${includeList} #include \"${hdr}\"\r\n")
        endforeach()
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/headerOnlyStub.cpp.in
            ${stubName})
        set_source_files_properties(${stubName} PROPERTIES GENERATED TRUE)
        list(APPEND sources ${stubName})
    endif()

    if (WIN32)
        set(resources_rc ${globbedFiles})
        list(FILTER resources_rc INCLUDE REGEX "\\.rc$")
        if (resources_rc)
            list(APPEND resources ${resources_rc})
        endif()
    endif()
    target_sources(${name} PRIVATE ${sources} ${headers} ${resources})
endfunction()

function(AddTarget)
    set(__options
        GENERATE_STUB             # 
        EXPORT_INCLUDES           # TARGET_INCLUDE_DIRECTORIES($SOURCE_DIR)
        EXPORT_PARENT_INCLUDES    # TARGET_INCLUDE_DIRECTORIES($SOURCE_DIR../)
        EXPORT_LINK
        STATIC_RUNTIME            # use static runtime (/MT) (MSVC)
        EXCLUDE_FROM_ALL          # 
        SKIP_STATIC_CHECK         #
        )
    set(__one_val_required
        NAME                # 
        TYPE                # shared|static|app_ui|app_console|header_only
        SOURCE_DIR          # 
        )
    set(__one_val_optional
        OUTPUT_NAME         #
        OUTPUT_PREFIX       #
        )
    set(__multi_val
        COMPILE_OPTIONS                # TARGET_COMPILE_OPTIONS(PRIVATE/INTERFACE)
        COMPILE_DEFINITIONS            # TARGET_COMPILE_DEFINITIONS(PRIVATE/INTERFACE)
        INTERFACE_COMPILE_DEFINITIONS  # TARGET_COMPILE_DEFINITIONS(INTERFACE)
        LINK_FLAGS                     # PROPERTIES LINK_FLAGS
        LINK_LIBRARIES                 # TARGET_LINK_LIBRARIES
        DEPENDENCIES                   # ADD_DEPENDENCIES
        INCLUDES                       # TARGET_INCLUDE_DIRECTORIES(PRIVATE/INTERFACE)
        INTERFACE_INCLUDES             # TARGET_INCLUDE_DIRECTORIES(INTERFACE)

        EXTRA_SOURCES                  # 
        EXCLUDE_SOURCES                # 
        QT_MODULES                     #
        UIC_POSTPROCESS_SCRIPTS        # 
    )
    ParseArgumentsWithConditions(ARG "${__options}" "${__one_val_required}" "${__one_val_optional}" "${__multi_val}" ${ARGN})

    set(name ${ARG_NAME})
    MakeTarget(${name} ${ARG_TYPE} "${ARG_EXCLUDE_FROM_ALL}")
    
    set(outputName ${name})
    if (ARG_OUTPUT_NAME)
        set(outputName ${ARG_OUTPUT_NAME})
    endif()
    if (ARG_OUTPUT_PREFIX)
        set(outputName ${ARG_OUTPUT_PREFIX}${outputName})
    endif()
    set_target_properties(${name} PROPERTIES OUTPUT_NAME "${outputName}" DEBUG_POSTFIX D)
    set_target_properties(${name} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib
    )

    if (NOT (ARG_TYPE STREQUAL interface))
        #function(MakeTargetSources name includes sourceDir useQt extraSources excludeSources extraPostprocess)
        set(useQt false)
        if(ARG_QT_MODULES)
            set(useQt true)
        endif()
        set(generateStub false)
        if(ARG_GENERATE_STUB)
            set(generateStub true)
        endif()

        target_include_directories(${name} PRIVATE ${CMAKE_BINARY_DIR}/export)
        MakeTargetSources(${name} "${ARG_INCLUDES}" "${ARG_SOURCE_DIR}" ${useQt} ${generateStub} "${ARG_EXTRA_SOURCES}" "${ARG_EXCLUDE_SOURCES}" "${ARG_UIC_POSTPROCESS_SCRIPTS}")
        if (NOT ARG_SKIP_STATIC_CHECK)
            AddStaticCheckTarget(TARGET_NAME ${name} SOURCE_DIR "${ARG_SOURCE_DIR}")
        endif()
    endif()

    set(defaultVisibility PRIVATE)
    if (ARG_TYPE STREQUAL interface)
        set(defaultVisibility INTERFACE)
    endif()
    if (ARG_TYPE STREQUAL header_only)
        set(defaultVisibility PUBLIC)
    endif()

    if (ARG_EXPORT_INCLUDES OR ARG_TYPE STREQUAL interface OR ARG_TYPE STREQUAL header_only)
        if (NOT (ARG_SOURCE_DIR STREQUAL .))
            target_include_directories(${name} INTERFACE ${ARG_SOURCE_DIR})
        endif()
    endif()
    if (ARG_EXPORT_PARENT_INCLUDES)
        target_include_directories(${name} INTERFACE ${ARG_SOURCE_DIR}/..)
    endif()

    if (ARG_INCLUDES)
        target_include_directories(${name} ${defaultVisibility} ${ARG_INCLUDES})
    endif()
    if (ARG_LINK_LIBRARIES)
        target_link_libraries(${name} ${defaultVisibility} ${ARG_LINK_LIBRARIES})
        if (ARG_EXPORT_LINK)
            target_link_libraries(${name} PUBLIC ${ARG_LINK_LIBRARIES})
        endif()
    endif()
    if (ARG_COMPILE_OPTIONS)
        target_compile_options(${name} ${defaultVisibility} ${ARG_COMPILE_OPTIONS})
    endif()
    if (ARG_COMPILE_DEFINITIONS)
        target_compile_definitions(${name} ${defaultVisibility} ${ARG_COMPILE_DEFINITIONS})
    endif()
    
    if (ARG_INTERFACE_INCLUDES)
        target_include_directories(${name} INTERFACE ${ARG_INTERFACE_INCLUDES})
    endif()
    if (ARG_INTERFACE_COMPILE_DEFINITIONS)
        target_compile_definitions(${name} INTERFACE ${ARG_INTERFACE_COMPILE_DEFINITIONS})
    endif()
    
    foreach(qt ${ARG_QT_MODULES})
        target_link_libraries(${name} ${defaultVisibility} Qt::${qt})
    endforeach()
    if (ARG_LINK_FLAGS)
        set_target_properties(${name} PROPERTIES LINK_FLAGS "${ARG_LINK_FLAGS}")
    endif()
    
    set(installableTypes shared app_ui app_console app_bundle)
    if (ARG_TYPE IN_LIST installableTypes AND NOT ARG_EXCLUDE_FROM_ALL)
        install(TARGETS ${name} RUNTIME DESTINATION bin)
    endif()
    if (MSVC AND ARG_STATIC_RUNTIME)
        target_link_libraries(${name} PRIVATE optimized libcmt.lib debug libcmtd.lib)
        target_compile_options(${name} PRIVATE $<$<CONFIG:Debug>:/MTd>$<$<CONFIG:Release>:/MT>)
    endif()
endfunction()
