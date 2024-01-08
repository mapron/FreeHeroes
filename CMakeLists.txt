# Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
# SPDX-License-Identifier: MIT
# See LICENSE file for details.

# Common declarations
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/install CACHE STRING "" FORCE)

cmake_minimum_required(VERSION 3.20)

project(freeheroes)
set(MERNEL_ROOT ${CMAKE_CURRENT_LIST_DIR}/ext/mernel)
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake ${MERNEL_ROOT}/cmake)

#project options
option( WARNING_AS_ERROR "" ON )
option( DISABLE_QWIDGET "" OFF )
option( DISABLE_QT "" OFF )
option( USE_SANITIZER "" OFF )
option( DISABLE_STATIC_CHECKS "" OFF )
set(FFMPEG_BINARY "" CACHE FILEPATH "Path to ffmpeg binary")
if ((NOT DISABLE_QWIDGET) AND DISABLE_QT)
    message("Disabling QWIDGET!")
    set(DISABLE_QWIDGET ON CACHE BOOL "" FORCE)
endif()

find_package(Git REQUIRED)

execute_process(
    COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/GitCommit.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/export/GitCommit.hpp)
if (MSVC AND USE_SANITIZER)
    add_compile_options(/fsanitize=address)
endif()

#[[
########## Compile flags ##########
#]]
include(utils)

#handling options
if (WARNING_AS_ERROR)
    if (MSVC)
        add_compile_options(/WX) #warning as error.
    else()
        add_compile_options(-Werror
            -Wno-error=sign-compare
            -Wno-error=redundant-move
            -Wno-error=implicit-fallthrough
            )
        AddCompilerFlagIfSupported(
            -Wno-error=implicit-int-float-conversion
            -Wno-error=class-memaccess
            -Wno-error=uninitialized
            -Wno-error=maybe-uninitialized
            -Wno-error=restrict
            -Wno-error=unused-function
            -Wno-error=deprecated-declarations
        )
    endif()
endif()

if (WIN32)
    add_definitions(-DNOMINMAX -D_UNICODE -DUNICODE -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES)
endif()

if (MSVC)
    add_compile_options(/W4
        /wd4267 # conversion from int to another int, possible losts of data
        /wd4251 # 'A::Impl' needs to have dll-interface to be used by clients of class 'A'
        /wd4275 # non dll-interface class 'Ismth' used as base for dll-interface class 'smth'
        /wd4127 # conditional expression is constant
        /wd5054 # operator '|': deprecated between enumerations of different types
        /wd4244 # conversion from 'int' to 'char', possible loss of data
        /wd4245 # '=': conversion from 'int' to 'unsigned int', signed/unsigned mismatch
        /wd4100 # 'data': unreferenced formal parameter
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

if (NOT DISABLE_QT)

set(QT_MODULE_LIST Core Gui Widgets Multimedia Network)
set(QT_MODULE_LIST_BUILD_ONLY ${QT_MODULE_LIST} LinguistTools)
if (DISABLE_QWIDGET)
    set(QT_MODULE_LIST Core Gui)
    set(QT_MODULE_LIST_BUILD_ONLY ${QT_MODULE_LIST})
endif()

find_package(Qt6 COMPONENTS ${QT_MODULE_LIST_BUILD_ONLY})

if (Qt6_FOUND)
    list(APPEND QT_MODULE_LIST Core5Compat)
    get_target_property(QT_MOC_EXECUTABLE ${QT_CMAKE_EXPORT_NAMESPACE}::moc IMPORTED_LOCATION_RELWITHDEBINFO)
    find_package(Qt6 COMPONENTS Core5Compat)
    
    if (MSVC)
        add_compile_options(/wd4702) # unreachable code in qanystringview.h isAsciiOnlyCharsAtCompileTime
    endif()
else()
    find_package(Qt5 COMPONENTS ${QT_MODULE_LIST_BUILD_ONLY} REQUIRED)
    
    set(QT_CMAKE_EXPORT_NAMESPACE Qt5)
    set(${QT_CMAKE_EXPORT_NAMESPACE}::uic ${Qt5Widgets_UIC_EXECUTABLE})
    set(${QT_CMAKE_EXPORT_NAMESPACE}::rcc ${Qt5Core_RCC_EXECUTABLE})
    set(${QT_CMAKE_EXPORT_NAMESPACE}::lrelease ${Qt5_LRELEASE_EXECUTABLE})
endif()

endif()

include(targets)
include(qt_helpers)


#[[
########## Mernel ##########
#]]
include(ext/mernel/mernel_thirdparty.cmake)

include(ext/mernel/mernel.cmake)

# Lua and Sol
AddTarget(TYPE static NAME lua_static SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lua/src
   # SRC
   # *.c *.h ../../sol/include/sol/*.hpp
    INTERFACE_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lua/include
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lua/include
    COMPILE_DEFINITIONS [ UNIX LUA_USE_POSIX ]
    COMPILE_OPTIONS
        [ MSVC
        /wd4310 # cast truncates constant value
        /wd4701 # potentially uninitialized local variable 'x' used
        /wd4702 # unreachable code
        /wd4324 # 'x': structure was padded due to alignment specifier
        ]
    SKIP_STATIC_CHECK
    )
#target_include_directories(lua_static INTERFACE 3rdparty/lua/include)



AddTarget(TYPE interface NAME sol SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/sol/include LINK_LIBRARIES lua_static)


AddTarget(TYPE interface NAME stb SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/stb)

#[[
########## FreeHeroes code targets ##########
#]]

# Interfaces
AddTarget(TYPE header_only GENERATE_STUB NAME GameObjects OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/GameObjects)
AddTarget(TYPE header_only GENERATE_STUB NAME SoundInt OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Sound/Interface
    )

# Core


AddTarget(TYPE header_only GENERATE_STUB NAME GameInt OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/GameInterface
    LINK_LIBRARIES GameObjects MernelPlatform)
if (NOT DISABLE_QT)
AddTarget(TYPE header_only GENERATE_STUB NAME GuiInt OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/Interface
    LINK_LIBRARIES MernelPlatform GameObjects GameInt  OUTPUT_PREFIX FH
    [ NOT DISABLE_QT QT_MODULES Gui ]
) # todo: GameObjects remove for cursors.
endif()

AddTarget(TYPE shared NAME CoreRng OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Rng
    EXPORT_INCLUDES
    LINK_LIBRARIES MernelPlatform GameInt
    )

AddTarget(TYPE shared NAME CoreResource OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Resource
    EXPORT_INCLUDES
    LINK_LIBRARIES MernelPlatform GameInt
    )

AddTarget(TYPE shared NAME CoreLogic OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Logic
    EXPORT_INCLUDES
    LINK_LIBRARIES CoreResource MernelPlatform GameObjects GameInt sol MernelReflection
    )

AddTarget(TYPE shared NAME BattleLogic OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/BattleLogic
    EXPORT_INCLUDES
    LINK_LIBRARIES MernelPlatform CoreLogic GameObjects GameInt
    )

AddTarget(TYPE shared NAME CoreApplication OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Application
    EXPORT_INCLUDES
    LINK_LIBRARIES
        MernelPlatform
        GameObjects
        GameInt
    
        CoreResource
        CoreLogic
        CoreRng
    )

# Sound
if (NOT DISABLE_QWIDGET)
AddTarget(TYPE shared NAME SoundPlayer OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Sound/Player
    EXPORT_INCLUDES
    LINK_LIBRARIES SoundInt CoreResource MernelPlatform GameInt
    QT_MODULES Multimedia
    )
endif()

# Gui
set(assetsPath ../../../assets)
AddTarget(TYPE shared NAME GuiResource OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/Resource
    EXPORT_INCLUDES
    LINK_LIBRARIES
        MernelPlatform CoreLogic GameInt
        MernelReflection
        stb

        [ NOT DISABLE_QT GuiInt ]
        [ NOT DISABLE_QT QT_MODULES Gui ]
    COMPILE_DEFINITIONS [ DISABLE_QT DISABLE_QT ]
    )

if (NOT DISABLE_QWIDGET)
AddTarget(TYPE shared NAME GuiGameWrappers OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/GameWrappers
    EXPORT_INCLUDES
    LINK_LIBRARIES
        GuiInt MernelPlatform GameInt SoundInt
        GameObjects GuiResource CoreResource GuiUtils
    QT_MODULES Widgets
    UIC_POSTPROCESS_SCRIPTS
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DepsUIPostProcessor.cmake
    )
endif()

if (NOT DISABLE_QT)
AddTarget(TYPE shared NAME GuiUtils OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/Utils
    EXPORT_INCLUDES
    LINK_LIBRARIES
        GuiInt
    QT_MODULES Core
    )
endif()

if (NOT DISABLE_QWIDGET)
AddTarget(TYPE shared NAME GuiWidgets OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/Widgets
    EXPORT_INCLUDES
    LINK_LIBRARIES
        GuiInt MernelPlatform GameObjects GameInt SoundInt

        GuiGameWrappers GuiUtils GuiResource
    QT_MODULES Widgets
    UIC_POSTPROCESS_SCRIPTS
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DepsUIPostProcessor.cmake
    )

AddTarget(TYPE shared NAME GuiEditor OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/Editor
    EXPORT_INCLUDES
    LINK_LIBRARIES
        GuiInt MernelPlatform GameObjects GameInt SoundInt
        GuiGameWrappers

        GuiWidgets GuiUtils GuiResource
    QT_MODULES Widgets
    UIC_POSTPROCESS_SCRIPTS
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DepsUIPostProcessor.cmake
    )

AddTarget(TYPE shared NAME GuiBattle OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/Battle
    EXPORT_INCLUDES
    LINK_LIBRARIES
        GuiInt MernelPlatform SoundInt GameObjects GameInt GuiResource

        CoreLogic
        GuiUtils
        GuiWidgets
        GuiGameWrappers
    QT_MODULES Widgets
    UIC_POSTPROCESS_SCRIPTS
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DepsUIPostProcessor.cmake
    )

AddTarget(TYPE shared NAME GuiApplication OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/Application
    EXPORT_INCLUDES
    LINK_LIBRARIES
        MernelPlatform
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
        GuiGameWrappers
        SoundPlayer
    QT_MODULES Widgets
    UIC_POSTPROCESS_SCRIPTS
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DepsUIPostProcessor.cmake
    )

endif()

AddTarget(TYPE shared NAME MapUtil OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/MapUtil
    EXPORT_INCLUDES
    LINK_LIBRARIES
        MernelPlatform
        GameObjects
        GameInt

        CoreLogic
        BattleLogic
        MernelReflection
    )

AddTarget(TYPE shared NAME MapRenderUtil OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/MapRenderUtil
    EXPORT_INCLUDES
    LINK_LIBRARIES
        MernelPlatform
        MernelReflection
        GuiResource
        GameObjects
        GameInt

        CoreLogic
        MapUtil
        
        [ NOT DISABLE_QT QT_MODULES Gui ]
    COMPILE_DEFINITIONS [ DISABLE_QT DISABLE_QT ]
    )

if (NOT DISABLE_QWIDGET)
AddTarget(TYPE shared NAME MapUtilGui OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/MapUtilGui
    EXPORT_INCLUDES
    LINK_LIBRARIES
        MernelPlatform
        MernelReflection
        GuiInt
        SoundInt
        GameObjects
        GameInt

        GuiWidgets
        GuiEditor
        GuiBattle
        GuiGameWrappers
        CoreLogic
        MapUtil
        MapRenderUtil
        GuiResource
    QT_MODULES Widgets
    UIC_POSTPROCESS_SCRIPTS
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DepsUIPostProcessor.cmake
    )
endif()

AddTarget(TYPE shared NAME LegacyConverterUtil OUTPUT_PREFIX FH
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/LegacyConverterUtil
    EXPORT_INCLUDES
    LINK_LIBRARIES
        CoreLogic
        MernelPlatform
        GameInt
        [ NOT DISABLE_QT GuiUtils GuiInt ]
        GuiResource
        GameObjects
    
        CoreResource
    
        ${PTHREAD} MernelReflection MernelExecution
        
        [ NOT DISABLE_QT QT_MODULES Gui ]
    COMPILE_DEFINITIONS [ DISABLE_QT DISABLE_QT ]
    )

# App
if (NOT DISABLE_QWIDGET)
AddTarget( TYPE app_ui NAME LegacyConverter
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/App/LegacyConverter
    LINK_LIBRARIES
        MernelPlatform
        GuiInt
        GameInt
        GameObjects
        GuiUtils
    
        CoreResource
        CoreApplication
        GuiApplication
        GuiResource
        LegacyConverterUtil

    QT_MODULES Widgets
    UIC_POSTPROCESS_SCRIPTS
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DepsUIPostProcessor.cmake
    )
endif()
AddTarget( TYPE app_console NAME LegacyConverterCLI
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/App/LegacyConverterCLI
    LINK_LIBRARIES
        MernelPlatform
        GameInt
        GameObjects
    
        CoreResource
        CoreApplication
        LegacyConverterUtil

        [ NOT DISABLE_QT QT_MODULES Gui ]
        COMPILE_DEFINITIONS [ DISABLE_QT DISABLE_QT ]
    )
AddTarget( TYPE app_console NAME GameLegacyConverterCLI
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/App/GameLegacyConverterCLI
    LINK_LIBRARIES
        MernelPlatform
        GameInt
        GameObjects
    
        CoreResource
        CoreApplication
        LegacyConverterUtil
        
        [ NOT DISABLE_QT GuiUtils ]
        [ NOT DISABLE_QT QT_MODULES Gui ]
        COMPILE_DEFINITIONS [ DISABLE_QT DISABLE_QT ]
    )
if (NOT DISABLE_QWIDGET)
AddTarget(TYPE app_ui NAME BattleEmulator
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/App/BattleEmulator
    LINK_LIBRARIES
        MernelPlatform
        GuiInt
        SoundInt
        GameObjects
        GameInt
        GuiResource

        CoreApplication
        GuiApplication
        GuiWidgets
        GuiEditor
        GuiBattle
        GuiGameWrappers
        CoreLogic
        BattleLogic
    QT_MODULES Widgets
    WIN_ICON
        ${CMAKE_CURRENT_SOURCE_DIR}/guiAssets/64_battle.ico
    UIC_POSTPROCESS_SCRIPTS
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DepsUIPostProcessor.cmake
    )

AddTarget(TYPE app_ui NAME MapEditor
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/App/MapEditor
    LINK_LIBRARIES
        MernelPlatform
        GuiUtils

        CoreApplication
        GuiApplication
        GameInt
        GuiInt
        MapUtilGui
    QT_MODULES Widgets
    WIN_ICON
        ${CMAKE_CURRENT_SOURCE_DIR}/guiAssets/64_map.ico
    )
endif()

if (NOT DISABLE_QWIDGET) #todo: should be fine without widget, remove MapEditor
AddTarget(TYPE app_ui NAME MapScreenshotToolCLI
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/App/MapScreenshotToolCLI
    LINK_LIBRARIES
        MernelPlatform
        GuiUtils

        CoreApplication
        GuiApplication
        GameInt
        GuiInt
        MapUtilGui
    QT_MODULES Widgets
    )
endif()
if (NOT DISABLE_QWIDGET)
AddTarget(TYPE app_ui NAME MapToolUI
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/App/MapToolUI
    LINK_LIBRARIES
        MernelPlatform
        MernelReflection
        GuiInt
        GuiUtils
        SoundInt
        GameObjects
        GameInt
        GuiResource

        CoreApplication
        GuiApplication
        GuiWidgets
        GuiEditor
        GuiBattle
        GuiGameWrappers
        CoreLogic
        BattleLogic
        MapUtil
        MapUtilGui
    QT_MODULES Widgets
    WIN_ICON
        ${CMAKE_CURRENT_SOURCE_DIR}/guiAssets/64_map.ico
    UIC_POSTPROCESS_SCRIPTS
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DepsUIPostProcessor.cmake
    )

endif()

AddTarget(TYPE app_console NAME MapToolCLI
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/App/MapToolCLI
    LINK_LIBRARIES
        MernelPlatform
        GameObjects
        GameInt

        CoreApplication
        MapUtil
    )

AddTarget(TYPE app_console NAME TemplateToolCLI
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/App/TemplateToolCLI
    LINK_LIBRARIES
        MernelPlatform
        GameObjects
        GameInt

        CoreApplication
        MapUtil
    )

if (NOT DISABLE_QWIDGET)
AddTarget(TYPE app_ui NAME Launcher
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/App/Launcher
    LINK_LIBRARIES
        GuiInt
        MernelPlatform
    QT_MODULES Widgets
    WIN_ICON
        ${CMAKE_CURRENT_SOURCE_DIR}/guiAssets/64.ico
    UIC_POSTPROCESS_SCRIPTS
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DepsUIPostProcessor.cmake
    )
endif()

if (WIN32 AND NOT DISABLE_QWIDGET)
AddTarget(TYPE app_ui NAME PostInstallSetup
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/App/PostInstallSetup
    LINK_LIBRARIES
        GuiInt
        MernelPlatform
    QT_MODULES Widgets
    UIC_POSTPROCESS_SCRIPTS
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DepsUIPostProcessor.cmake
    )
endif()

# api bindings

AddTarget(TYPE shared NAME FreeHeroesAPI
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/API/FreeHeroesAPI
    LINK_LIBRARIES
        MernelPlatform
    
        GameObjects
        GameInt
    
        CoreResource
        CoreLogic
        CoreRng
        GameInt
        GuiResource
        MapUtil
        MapRenderUtil
        LegacyConverterUtil
    )

AddTarget(TYPE app_console NAME FreeHeroesAPITest
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/API/FreeHeroesAPITest
        INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/API/FreeHeroesAPI
        LINK_LIBRARIES
            MernelPlatform # just for SharedLibLoader.
    )
add_dependencies(FreeHeroesAPITest FreeHeroesAPI) # build-only.

# tests

AddTarget(TYPE app_console NAME CoreTests OUTPUT_NAME Tests_CoreLogic
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Tests
    LINK_LIBRARIES
        MernelPlatform
        GameObjects
        GameInt

        CoreLogic
        MapUtil

    gtest gtest_main MernelReflection
    )

if (NOT DISABLE_QWIDGET)
AddTarget(TYPE app_ui NAME SoundTests OUTPUT_NAME Tests_Sound
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Sound/Tests
    LINK_LIBRARIES
        MernelPlatform
        SoundInt
        GameInt

        CoreApplication
        GuiApplication

    QT_MODULES Widgets
    UIC_POSTPROCESS_SCRIPTS
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DepsUIPostProcessor.cmake
    )
endif()

if (NOT DISABLE_QWIDGET)
# Qt UI resource files = qrc + ts/qm
GenerateQrcFromAssets(Application)
GenerateQrcFromAssets(Battle)
GenerateQrcWithTranslations(Translations ${CMAKE_CURRENT_SOURCE_DIR}/guiAssets/TranslationSource )
AddResourceCustomTarget("all_resources" Application Battle Translations)
endif()

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

include(installer)


