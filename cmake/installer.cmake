
#[[
########## Installation (Win32 only at the moment) ##########
#]]

if(NOT WIN32)
    return()
endif()

# Qt install - plugins
set(QtPluginsList)
if (NOT DISABLE_QWIDGET)
set(QtPluginsList ${QT_CMAKE_EXPORT_NAMESPACE}::QWebpPlugin ${${QT_CMAKE_EXPORT_NAMESPACE}Multimedia_PLUGINS})
endif()

if (NOT DISABLE_QWIDGET)
if(APPLE)
    list(APPEND QtPluginsList ${QT_CMAKE_EXPORT_NAMESPACE}::QCocoaIntegrationPlugin)
elseif(WIN32)
    list(APPEND QtPluginsList ${QT_CMAKE_EXPORT_NAMESPACE}::QWindowsIntegrationPlugin ${QT_CMAKE_EXPORT_NAMESPACE}::QWindowsVistaStylePlugin)
endif()
list(REMOVE_ITEM QtPluginsList ${QT_CMAKE_EXPORT_NAMESPACE}::QM3uPlaylistPlugin)

if (QT_CMAKE_EXPORT_NAMESPACE STREQUAL Qt6)
    list(REMOVE_ITEM QtPluginsList ${QT_CMAKE_EXPORT_NAMESPACE}::QWebpPlugin)
endif()
else()
    if(WIN32)
        list(APPEND QtPluginsList ${QT_CMAKE_EXPORT_NAMESPACE}::QWindowsIntegrationPlugin)
    endif()
endif()

if (DISABLE_QT)
    set(QtPluginsList)
endif()

foreach(plugin ${QtPluginsList})
    get_target_property(imploc_RELEASE ${plugin} IMPORTED_LOCATION_RELEASE)
    get_target_property(imploc_DEBUG   ${plugin} IMPORTED_LOCATION_DEBUG)

    get_filename_component(pluginPath ${imploc_RELEASE} DIRECTORY)
    get_filename_component(pluginDir ${pluginPath} NAME)
    if (EXISTS "${imploc_DEBUG}")
        install(FILES ${imploc_DEBUG}   DESTINATION bin/${pluginDir} CONFIGURATIONS Debug )
    endif()
    install(FILES ${imploc_RELEASE} DESTINATION bin/${pluginDir} CONFIGURATIONS Release RelWithDebInfo )
endforeach()

# Qt install - shared libs
foreach(Qt6Module ${QT_MODULE_LIST})
    get_target_property(imploc_RELEASE ${QT_CMAKE_EXPORT_NAMESPACE}::${Qt6Module} IMPORTED_LOCATION_RELEASE)
    get_target_property(imploc_DEBUG   ${QT_CMAKE_EXPORT_NAMESPACE}::${Qt6Module} IMPORTED_LOCATION_DEBUG)
    if (EXISTS "${imploc_DEBUG}")
        install(FILES ${imploc_DEBUG}   DESTINATION bin CONFIGURATIONS Debug    )
    endif()
    install(FILES ${imploc_RELEASE} DESTINATION bin CONFIGURATIONS Release RelWithDebInfo )
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
    WIN_ICON
        ${CMAKE_CURRENT_SOURCE_DIR}/guiAssets/64.ico
)
add_dependencies(InstallerSelfExtractor InstallArchive)
