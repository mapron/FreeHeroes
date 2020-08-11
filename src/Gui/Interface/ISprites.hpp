/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QList>
#include <QPixmap>

#include <memory>


namespace FreeHeroes::Gui {

struct SpriteFrame
{
    QPixmap frame;
    int32_t id = 0;
    bool duplicate = false;
    QPoint paddingLeftTop;
};

struct SpriteSequenceParams
{
    int scaleFactorPercent = 100;
    int animationCycleDuration = 1000;
    int specialFrameIndex = -1;
    QPoint actionPoint;
};

struct SpriteSequence
{
    QList<SpriteFrame> frames;
    QSize boundarySize;
    SpriteSequenceParams params;
};

using SpriteSequencePtr = std::shared_ptr<const SpriteSequence>;

class ISprite
{
public:
    virtual ~ISprite() = default;
    virtual int getGroupsCount() const = 0;

    virtual QList<int> getGroupsIds() const = 0;

    virtual SpriteSequencePtr getFramesForGroup(int group) const = 0;
};

using SpritePtr = std::shared_ptr<const ISprite>;

}
