/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiUtilsExport.hpp"

#include <QList>
#include <QVariant>

#include <functional>
#include <memory>

class QSequentialAnimationGroup;
class QAnimationGroup;
class QObject;

namespace FreeHeroes::Gui {

class GUIUTILS_EXPORT AnimationTree {
public:
    AnimationTree();
    ~AnimationTree();

    void addPropertyAnimation(QObject* target, int msecs, const QByteArray& propertyName, const QVariant& endValue);

    void addCallback(std::function<void()> callback, int pauseDuration);

    void runSync(bool excludeInput);
    void runAsync();

    void beginParallel();
    void beginSequental();
    void endGroup();
    void pause(int msec);

private:
    std::unique_ptr<QSequentialAnimationGroup> m_root;
    QAnimationGroup*                           m_currentGroup;
    QList<QAnimationGroup*>                    m_startedGroups;
};

}
