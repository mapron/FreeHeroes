include_guard(GLOBAL)

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
                           COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::lrelease
                           ARGS ${tsFileAbs} -qm ${qmFileAbs}
                           MAIN_DEPENDENCY ${tsFileAbs} VERBATIM)

        string(REPLACE "${resourceFolder}/" "" file "${relFile}")
        set(QRC_FILES "${QRC_FILES}<file alias=\"${file}\">${resourceFolder}/${relFile}</file>\n")
    endforeach()
    AddQrcOutput(${rccName} ${qrcName} ${fileListAbsConfig})
endfunction()
