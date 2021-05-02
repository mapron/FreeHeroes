/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "DependencyInjector.hpp"

#include <QApplication>

namespace FreeHeroes::Gui {

namespace {

QVariant getTopLevelData(const char* propName)
{
    for (QWidget* topLevelWidget : QApplication::topLevelWidgets()) {
        QVariant data = topLevelWidget->property(propName);
        if (data.isValid())
            return data;
    }
    return QVariant();
}

}

QVariant loadDependencyData(QWidget* widget, const char* propName)
{
    if (!widget) {
        QVariant data = getTopLevelData(propName);
        if (data.isValid())
            return data;
    }
    QVariant data = widget->property(propName);
    if (data.isValid())
        return data;
    if (!widget->isWindow())
        return loadDependencyData(widget->window(), propName);
    if (widget->parent())
        return loadDependencyData(widget->parentWidget(), propName);

    data = getTopLevelData(propName);
    if (data.isValid())
        return data;

    throw std::runtime_error("Dependency data is not initialized:" + std::string(propName));
    return QVariant();
}

}
