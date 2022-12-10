# Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
# SPDX-License-Identifier: MIT
# See LICENSE file for details.

# Common declarations
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/install CACHE STRING "" FORCE)

cmake_minimum_required(VERSION 3.20)

project(freeheroes)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

#project options
option( WARNING_AS_ERROR "" ON )
set(FFMPEG_BINARY "" CACHE FILEPATH "Path to ffmpeg binary")

include(targets)

#[[
########## Compile flags ##########
#]]

#handling options
if (WARNING_AS_ERROR)
    if (MSVC)
        add_compile_options(/WX) #warning as error.
    else()
        # most of exceptions - workaround for json/rttr headers.
        add_compile_options(-Werror
            -Wno-error=sign-compare
            -Wno-error=redundant-move
            -Wno-error=implicit-fallthrough
            )
        AddCompilerFlagIfSupported(-Wno-error=implicit-int-float-conversion)
    endif()
endif()

if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
    add_compile_options(/W3
        /wd4267 # conversion from int to another int, possible losts of data
        /wd4251 # 'A::Impl' needs to have dll-interface to be used by clients of class 'A'
        /wd4275 # non dll-interface class 'Ismth' used as base for dll-interface class 'smth'
        /bigobj
        )
else()
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
    AddCompilerFlagIfSupported(-Wno-missing-field-initializers)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    set(PTHREAD Threads::Threads)
endif()

#[[
########## External dependencies section ##########
#]]

# ffmpeg
if (EXISTS "${FFMPEG_BINARY}")
    file(COPY ${FFMPEG_BINARY} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
endif()

# Qt Setup
# QtNetwork is only for QtMultimedia
set(QT_MODULE_LIST Core Gui Widgets Multimedia Network)
find_package(Qt5 COMPONENTS ${QT_MODULE_LIST} LinguistTools REQUIRED)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/mocWrapper.cmake.in ${CMAKE_BINARY_DIR}/mocWrapper.cmake @ONLY)

# Zlib

if (MSVC)
    AddTarget(TYPE static NAME zlib 
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zlib
        EXPORT_INCLUDES
        COMPILE_OPTIONS /wd4996
        INTERFACE_COMPILE_DEFINITIONS USE_ZLIB
    )
else()
    AddTarget(TYPE interface NAME zlib 
        SOURCE_DIR .
        LINK_LIBRARIES -lz
        INTERFACE_COMPILE_DEFINITIONS USE_ZLIB
    )
endif()

# Json
AddTarget(TYPE interface NAME rapidjson 
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)

# frozen 
AddTarget(TYPE interface NAME frozen 
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/frozen/include)

# Lua and Sol
AddTarget(TYPE static NAME lua_static SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lua/src
   # SRC
   # *.c *.h ../../sol/include/sol/*.hpp
    INTERFACE_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lua/include
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lua/include
    COMPILE_DEFINITIONS [ UNIX LUA_USE_POSIX ]
    )
#target_include_directories(lua_static INTERFACE 3rdparty/lua/include)



AddTarget(TYPE interface NAME sol SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/sol/include LINK_LIBRARIES lua_static)



# Google Test
set(BUILD_GMOCK OFF)
set(INSTALL_GTEST OFF)
set(GOOGLETEST_VERSION 1.10.0)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(3rdparty/googletest/googletest)

#[[
########## FreeHeroes code targets ##########
#]]

# Interfaces
AddTarget(TYPE header_only GENERATE_STUB NAME GameObjects OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/GameObjects)
AddTarget(TYPE header_only GENERATE_STUB NAME SoundInt OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Sound/Interface
    QT_MODULES Widgets)

# Core
AddTarget(TYPE shared NAME CorePlatform OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Platform
    EXPORT_INCLUDES
    LINK_LIBRARIES rapidjson zlib)

AddTarget(TYPE header_only GENERATE_STUB NAME GameInt OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/GameInterface
    LINK_LIBRARIES GameObjects CorePlatform)
AddTarget(TYPE header_only GENERATE_STUB NAME GuiInt OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/Interface
    LINK_LIBRARIES CorePlatform GameObjects GameInt  OUTPUT_PREFIX FH
    QT_MODULES Widgets) # todo: GameObjects remove for cursors.

AddTarget(TYPE shared NAME CoreRng OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Rng
    EXPORT_INCLUDES
    LINK_LIBRARIES CorePlatform
    )

AddTarget(TYPE shared NAME CoreResource OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Resource
    EXPORT_INCLUDES
    LINK_LIBRARIES CorePlatform
    )

AddTarget(TYPE shared NAME CoreLogic OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Logic
    EXPORT_INCLUDES
    LINK_LIBRARIES CoreResource CorePlatform GameObjects GameInt sol frozen
    )

AddTarget(TYPE shared NAME BattleLogic OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/BattleLogic
    EXPORT_INCLUDES
    LINK_LIBRARIES CorePlatform CoreLogic GameObjects GameInt
    )

AddTarget(TYPE shared NAME CoreApplication OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Application
    EXPORT_INCLUDES
    LINK_LIBRARIES
        CorePlatform
        GameObjects
        GameInt
    
        CoreResource
        CoreLogic
        CoreRng
    )

# Sound

AddTarget(TYPE shared NAME SoundPlayer OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Sound/Player
    EXPORT_INCLUDES
    LINK_LIBRARIES SoundInt CoreResource CorePlatform
    QT_MODULES Multimedia
    )


# Gui
set(assetsPath ../../../assets)
AddTarget(TYPE shared NAME GuiResource OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/Resource
    EXPORT_INCLUDES
    LINK_LIBRARIES
        GuiInt CorePlatform
    QT_MODULES Widgets
    )

AddTarget(TYPE shared NAME GuiLocalization OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/Localization
    EXPORT_INCLUDES
    LINK_LIBRARIES
        GuiInt CorePlatform GameObjects
    QT_MODULES Widgets
    )

AddTarget(TYPE shared NAME GuiGameWrappers OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/GameWrappers
    EXPORT_INCLUDES
    LINK_LIBRARIES
        GuiInt CorePlatform GameInt SoundInt
        GameObjects GuiResource CoreResource GuiUtils
    QT_MODULES Widgets
    )

AddTarget(TYPE shared NAME GuiUtils OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/Utils
    EXPORT_INCLUDES
    LINK_LIBRARIES
        GuiInt
    QT_MODULES Widgets
    )

AddTarget(TYPE shared NAME GuiWidgets OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/Widgets
    EXPORT_INCLUDES
    LINK_LIBRARIES
        GuiInt CorePlatform GameObjects GameInt SoundInt

        GuiGameWrappers GuiUtils
    QT_MODULES Widgets
    )

AddTarget(TYPE shared NAME GuiEditor OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/Editor
    EXPORT_INCLUDES
    LINK_LIBRARIES
        GuiInt CorePlatform GameObjects GameInt SoundInt
        GuiGameWrappers

        GuiWidgets GuiUtils
    QT_MODULES Widgets
    )

AddTarget(TYPE shared NAME GuiBattle OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/Battle
    EXPORT_INCLUDES
    LINK_LIBRARIES
        GuiInt CorePlatform SoundInt GameObjects GameInt

        CoreLogic
        GuiUtils
        GuiWidgets
        GuiGameWrappers
    QT_MODULES Widgets
    )

AddTarget(TYPE shared NAME GuiApplication OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/Application
    EXPORT_INCLUDES
    LINK_LIBRARIES
        CorePlatform
        GuiInt
        SoundInt
        GameObjects
        GameInt

        CoreApplication
        CoreResource
        CoreLogic
        CoreRng
        GuiBattle
        GuiWidgets
        GuiResource
        GuiLocalization
        GuiGameWrappers
        SoundPlayer
    QT_MODULES Widgets
    )

# App

AddTarget( TYPE app_ui NAME LegacyConverter
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/App/LegacyConverter
    LINK_LIBRARIES
    CorePlatform
    GuiInt
    GameInt
    GameObjects

    CoreResource
    CoreApplication
    GuiApplication
    GuiResource

    zlib ${PTHREAD}
    QT_MODULES Widgets
    )
AddTarget(TYPE app_ui NAME BattleEmulator
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/App/BattleEmulator
    LINK_LIBRARIES
        CorePlatform
        GuiInt
        SoundInt
        GameObjects
        GameInt

        CoreApplication
        GuiApplication
        GuiWidgets
        GuiEditor
        GuiBattle
        GuiGameWrappers
        CoreLogic
        BattleLogic
    QT_MODULES Widgets
    )

AddTarget(TYPE app_ui NAME MapGenTest
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/App/MapGenTest
    LINK_LIBRARIES
        CorePlatform
        GuiInt
        SoundInt
        GameObjects
        GameInt

        CoreApplication
        GuiApplication
        GuiWidgets
        GuiEditor
        GuiBattle
        GuiGameWrappers
        CoreLogic
        BattleLogic
    QT_MODULES Widgets
    )

AddTarget(TYPE app_console NAME MapUtils
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/App/MapUtils
    LINK_LIBRARIES
        CorePlatform
        GameObjects
        GameInt

        CoreApplication
        CoreLogic
        BattleLogic
        frozen
    )

# tests

AddTarget(TYPE app_console NAME CoreTests OUTPUT_NAME Tests_CoreLogic
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Tests
    LINK_LIBRARIES
        CorePlatform
        GameObjects
        GameInt

        CoreLogic

    gtest gtest_main frozen
    )

AddTarget(TYPE app_ui NAME SoundTests OUTPUT_NAME Tests_Sound
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Sound/Tests
    LINK_LIBRARIES
        CorePlatform
        SoundInt
        GameInt

        CoreApplication
        GuiApplication

    QT_MODULES Widgets
    )


# Qt UI resource files = qrc + ts/qm
GenerateQrcFromAssets(Application)
GenerateQrcFromAssets(Battle)
GenerateQrcWithTranslations(Translations ${CMAKE_CURRENT_SOURCE_DIR}/guiAssets/TranslationSource )
AddResourceCustomTarget("all_resources" Application Battle Translations)

#[[
########## Game Resources ##########
#]]

file(GLOB_RECURSE resourceFiles RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/
    ${CMAKE_CURRENT_SOURCE_DIR}/gameResources/*.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/gameResources/*.json
    ${CMAKE_CURRENT_SOURCE_DIR}/gameResources/*.png)
foreach(filename ${resourceFiles})
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${filename} ${CMAKE_CURRENT_BINARY_DIR}/${filename} COPYONLY)
endforeach()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/gameResources DESTINATION bin)

#[[
########## Installation (Win32 only at the moment) ##########
#]]

if(WIN32)
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
endif(WIN32)
