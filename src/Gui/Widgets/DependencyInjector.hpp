/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "ICursorLibrary.hpp"
#include "IMusicBox.hpp"
#include "IAppSettings.hpp"
#include "LibraryModels.hpp"

#include "GuiWidgetsExport.hpp"

#include <QWidget>
#include <QVariant>

#include <stdexcept>

Q_DECLARE_METATYPE(FreeHeroes::Gui::ICursorLibrary*)
Q_DECLARE_METATYPE(FreeHeroes::Gui::IAppSettings*)
Q_DECLARE_METATYPE(FreeHeroes::Sound::IMusicBox*)
Q_DECLARE_METATYPE(FreeHeroes::Gui::LibraryModelsProvider*)

namespace FreeHeroes::Gui {


inline void storeDependency(QWidget * widget, ICursorLibrary & library) {
    widget->setProperty("ICursorLibrary", QVariant::fromValue(&library));
}
inline void storeDependency(QWidget * widget, LibraryModelsProvider & library) {
    widget->setProperty("LibraryModelsProvider", QVariant::fromValue(&library));
}
inline void storeDependency(QWidget * widget, Sound::IMusicBox & musicBox) {
    widget->setProperty("IMusicBox", QVariant::fromValue(&musicBox));
}
inline void storeDependency(QWidget * widget, IAppSettings & appSettings) {
    widget->setProperty("IAppSettings", QVariant::fromValue(&appSettings));
}

GUIWIDGETS_EXPORT QVariant loadDependencyData(QWidget * widget, const char * propName);

template<typename T>
inline T& loadDependency(QWidget * ) {
    static_assert(sizeof(T)==3);
}

template<>
inline ICursorLibrary& loadDependency<ICursorLibrary>(QWidget * widget) {
    return *loadDependencyData(widget, "ICursorLibrary").value<ICursorLibrary*>();
}
template<>
inline LibraryModelsProvider& loadDependency<LibraryModelsProvider>(QWidget * widget) {
    return *loadDependencyData(widget, "LibraryModelsProvider").value<LibraryModelsProvider*>();
}
template<>
inline Sound::IMusicBox& loadDependency<Sound::IMusicBox>(QWidget * widget) {
    return *loadDependencyData(widget, "IMusicBox").value<Sound::IMusicBox*>();
}
template<>
inline IAppSettings& loadDependency<IAppSettings>(QWidget * widget) {
    return *loadDependencyData(widget, "IAppSettings").value<IAppSettings*>();
}

}
