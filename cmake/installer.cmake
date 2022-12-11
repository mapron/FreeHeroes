
#[[
########## Installation (Win32 only at the moment) ##########
#]]

if(NOT WIN32)
    return()
endif()

# Qt install - plugins
set(QtPluginsList Qt5::QWebpPlugin ${Qt5Multimedia_PLUGINS})
if(APPLE)
    list(APPEND QtPluginsList Qt5::QCocoaIntegrationPlugin)
elseif(WIN32)
    list(APPEND QtPluginsList Qt5::QWindowsIntegrationPlugin Qt5::QWindowsVistaStylePlugin)
endif()
list(REMOVE_ITEM QtPluginsList Qt5::QM3uPlaylistPlugin)
foreach(plugin ${QtPluginsList})
    get_target_property(imploc_RELEASE ${plugin} IMPORTED_LOCATION_RELEASE)
    get_target_property(imploc_DEBUG   ${plugin} IMPORTED_LOCATION_DEBUG)

    get_filename_component(pluginPath ${imploc_RELEASE} DIRECTORY)
    get_filename_component(pluginDir ${pluginPath} NAME)
    if (EXISTS "${imploc_DEBUG}")
        install(FILES ${imploc_DEBUG}   DESTINATION bin/${pluginDir} CONFIGURATIONS Debug )
    endif()
    install(FILES ${imploc_RELEASE} DESTINATION bin/${pluginDir} CONFIGURATIONS Release )
endforeach()

# Qt install - shared libs
foreach(qt5Module ${QT_MODULE_LIST})
    get_target_property(imploc_RELEASE Qt5::${qt5Module} IMPORTED_LOCATION_RELEASE)
    get_target_property(imploc_DEBUG   Qt5::${qt5Module} IMPORTED_LOCATION_DEBUG)
    if (EXISTS "${imploc_DEBUG}")
        install(FILES ${imploc_DEBUG}   DESTINATION bin CONFIGURATIONS Debug    )
    endif()
    install(FILES ${imploc_RELEASE} DESTINATION bin CONFIGURATIONS Release  )
endforeach()

# Compiler runtime
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
if (NOT(CMAKE_BUILD_TYPE STREQUAL Release))
    set(CMAKE_INSTALL_DEBUG_LIBRARIES TRUE)
endif()
include (InstallRequiredSystemLibraries)

# ffmpeg
if (EXISTS "${FFMPEG_BINARY}")
    install(FILES "${FFMPEG_BINARY}" DESTINATION bin)
endif()

AddInstallArchiveTarget(InstallArchive)

AddTarget(TYPE static NAME 7zip_static
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/7zip
    EXPORT_INCLUDES
    SKIP_STATIC_CHECK
    STATIC_RUNTIME
)

file(REMOVE ${CMAKE_BINARY_DIR}/installer_resource.rc)
file(REMOVE ${CMAKE_BINARY_DIR}/InstallData.7z)

set(archive ${CMAKE_BINARY_DIR}/InstallData.7z)
set(splash ${CMAKE_CURRENT_SOURCE_DIR}/src/InstallerExtractor/splash.bmp)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/InstallerExtractor/resources.rc.in ${CMAKE_BINARY_DIR}/installer_resource.rc)

AddTarget(TYPE app_ui NAME InstallerSelfExtractor OUTPUT_NAME FreeHeroes_Installer
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/InstallerExtractor
    LINK_LIBRARIES 7zip_static
    STATIC_RUNTIME
    EXCLUDE_FROM_ALL
    EXTRA_SOURCES
        ${CMAKE_BINARY_DIR}/installer_resource.rc
)
add_dependencies(InstallerSelfExtractor InstallArchive)
