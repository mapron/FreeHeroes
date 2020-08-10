/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "ISprites.hpp"

#include "GuiResourceExport.hpp"

#include <QMap>

namespace FreeHeroes::Gui {

class GUIRESOURCE_EXPORT Sprite : public ISprite
{
public:
    int getGroupsCount() const override {
        return m_groups.size();
    }

    QList<int> getGroupsIds() const override {
        return m_groups.keys();
    }

    SpriteSequencePtr getFramesForGroup(int group) const override {
        return m_groups.value(group);
    }

    static SpritePtr fromPixmap(QPixmap pixmap);

protected:
    void addGroup(int groupId, SpriteSequencePtr seq);

private:
    QMap<int, SpriteSequencePtr> m_groups;
};

}
