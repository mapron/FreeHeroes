# Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
# SPDX-License-Identifier: MIT
# See LICENSE file for details.

# Common declarations
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/install CACHE STRING "" FORCE)

cmake_minimum_required(VERSION 3.12)

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
    set(ZLIB_ROOT 3rdparty/zlib)
    AddTarget(TYPE static NAME zlib_static ROOT ${ZLIB_ROOT}/
        OPTIONS  /wd4996)
    set(COMPRESSION_LIBS zlib_static)
else()
    set(COMPRESSION_LIBS -lz)
endif()

# nlohmann Json
AddTargetInterface(NAME nlohmann_json INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/nlohmann_json)


# Lua and Sol
set(LUA_DEFINITIONS)
if(UNIX)
    list(APPEND LUA_DEFINITIONS LUA_USE_POSIX)
endif()

AddTarget(NO_DEFAULT_GLOB TYPE static NAME lua_static ROOT 3rdparty/lua/src
    SRC
    *.c *.h ../../sol/include/sol/*.hpp
    INCLUDES 3rdparty/lua/include
    )
target_include_directories(lua_static INTERFACE 3rdparty/lua/include)

AddTargetInterface(NAME sol INCLUDES 3rdparty/sol/include DEPS lua_static)


# RTTR

function(AddRTTR)
    set(RTTR_VERSION_MAJOR 0)
    set(RTTR_VERSION_MINOR 9)
    set(RTTR_VERSION_PATCH 7)
    set(RTTR_VERSION ${RTTR_VERSION_MAJOR}.${RTTR_VERSION_MINOR}.${RTTR_VERSION_PATCH})
    set(RTTR_VERSION_STR "${RTTR_VERSION_MAJOR}.${RTTR_VERSION_MINOR}.${RTTR_VERSION_PATCH}")
    set(root ${CMAKE_CURRENT_LIST_DIR}/3rdparty/rttr)
    configure_file(${root}/rttr/detail/base/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/rttr/detail/base/version.h)
    include(${root}/rttr/rttr.cmake)
    set(src)
    foreach(file ${HEADER_FILES} ${SOURCE_FILES})
        list(APPEND src ${root}/rttr/${file})
    endforeach()
    add_library(rttr STATIC ${src})
    target_include_directories(rttr PUBLIC ${root} ${CMAKE_CURRENT_BINARY_DIR})
endfunction()
AddRTTR()

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
AddTargetHeaderOnly(FH  NAME GameObjects        INCLUDES src/Core/GameObjects)
AddTargetHeaderOnly(FH  NAME GameInt            INCLUDES src/Core/GameInterface DEPS_FH GameObjects CorePlatform )
AddTargetHeaderOnly(FH  NAME GuiInt             INCLUDES src/Gui/Interface  DEPS_FH CorePlatform GameObjects DEPS Qt5::Widgets) # todo: GameObjects remove for cursors.
AddTargetHeaderOnly(FH  NAME SoundInt           INCLUDES src/Sound/Interface DEPS Qt5::Widgets)

# Core
AddTarget          (FH  NAME CorePlatform  TYPE shared   ROOT src/Core/Platform )

AddTarget(FH TYPE shared NAME CoreRng ROOT src/Core/Rng
    DEPS_FH CorePlatform
    )

AddTarget(FH TYPE shared NAME CoreResource ROOT src/Core/Resource
    DEPS_FH CorePlatform
    DEPS  nlohmann_json
    )

AddTarget(FH TYPE shared NAME CoreLogic ROOT src/Core/Logic
    DEPS_FH CoreResource CorePlatform GameObjects GameInt
    DEPS  nlohmann_json sol rttr
    )

AddTarget(FH TYPE shared NAME BattleLogic ROOT src/Core/BattleLogic
    DEPS_FH CorePlatform CoreLogic GameObjects GameInt
    )

# Sound

AddTarget(FH QT TYPE shared NAME SoundPlayer ROOT src/Sound/Player
    DEPS_FH SoundInt CoreResource CorePlatform
    DEPS  Qt5::Multimedia
    )


# Gui
set(assetsPath ../../../assets)
AddTarget(FH QT TYPE shared NAME GuiResource ROOT src/Gui/Resource
    DEPS_FH
        GuiInt CorePlatform
    DEPS
        Qt5::Widgets nlohmann_json
    )

AddTarget(FH QT TYPE shared NAME GuiLocalization ROOT src/Gui/Localization
    DEPS_FH
        GuiInt CorePlatform GameObjects
    DEPS
        Qt5::Widgets
    )

AddTarget(FH QT TYPE shared NAME GuiGameWrappers ROOT src/Gui/GameWrappers
    DEPS_FH
        GuiInt CorePlatform GameInt SoundInt
        GameObjects GuiResource CoreResource GuiUtils
    DEPS
        Qt5::Widgets
    )

AddTarget(FH QT TYPE shared NAME GuiUtils ROOT src/Gui/Utils
    DEPS_FH
        GuiInt
    DEPS
        Qt5::Widgets
    )

AddTarget(FH QT TYPE shared NAME GuiWidgets ROOT src/Gui/Widgets
    DEPS_FH
        GuiInt CorePlatform GameObjects GameInt SoundInt

        GuiGameWrappers GuiUtils
    DEPS
        Qt5::Widgets
    )

AddTarget(FH QT TYPE shared NAME GuiEditor ROOT src/Gui/Editor
    DEPS_FH
        GuiInt CorePlatform GameObjects GameInt SoundInt
        GuiGameWrappers

        GuiWidgets GuiUtils
    DEPS
        Qt5::Widgets
    )

AddTarget(FH QT TYPE shared NAME GuiBattle ROOT src/Gui/Battle
    DEPS_FH
        GuiInt CorePlatform SoundInt GameObjects GameInt

        CoreLogic
        GuiUtils
        GuiWidgets
        GuiGameWrappers
    DEPS
        Qt5::Widgets
    )

AddTarget(FH QT TYPE shared NAME GuiApplication ROOT src/Gui/Application
    DEPS_FH
        CorePlatform
        GuiInt
        SoundInt
        GameObjects
        GameInt

        CoreResource
        CoreLogic
        CoreRng
        GuiBattle
        GuiWidgets
        GuiResource
        GuiLocalization
        GuiGameWrappers
        SoundPlayer
    DEPS  Qt5::Widgets nlohmann_json
    )

# App

AddTarget(FH QT  TYPE app NAME LegacyConverter ROOT src/App/LegacyConverter
    DEPS_FH
    CorePlatform
    GuiInt
    GameObjects

    CoreResource
    GuiApplication
    GuiResource

    DEPS Qt5::Widgets nlohmann_json ${COMPRESSION_LIBS} ${PTHREAD}
    )
AddTarget(FH QT TYPE app NAME BattleEmulator
    ROOT src/App/BattleEmulator
    DEPS_FH
        CorePlatform
        GuiInt
        SoundInt
        GameObjects
        GameInt

        GuiApplication
        GuiWidgets
        GuiEditor
        GuiBattle
        GuiGameWrappers
        CoreLogic
        BattleLogic
    DEPS Qt5::Widgets
    )



# tests

AddTarget(FH TYPE console_app NAME CoreTests OUTPUT_NAME Tests_CoreLogic
    ROOT src/Core/Tests
    DEPS_FH
        CorePlatform
        GameObjects
        GameInt

        CoreLogic
    DEPS gtest gtest_main
    )

AddTarget(FH TYPE app NAME SoundTests OUTPUT_NAME Tests_Sound
    ROOT src/Sound/Tests
    DEPS_FH
        CorePlatform
        SoundInt

        GuiApplication
    DEPS Qt5::Widgets
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
