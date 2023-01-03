/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "ISprites.hpp"

#include "GuiResourceExport.hpp"
#include "MernelPlatform/FsUtils.hpp"

#include <QMap>

namespace FreeHeroes::Gui {

struct GUIRESOURCE_EXPORT Sprite : public ISprite {
    QPixmap m_bitmap;

    struct Frame {
        QPoint m_padding;
        bool   m_hasBitmap = true;

        QSize  m_bitmapSize;
        QPoint m_bitmapOffset;

        QSize m_boundarySize;
    };
    struct Params {
        int    m_scaleFactorPercent     = 100;
        int    m_animationCycleDuration = 1000;
        int    m_specialFrameIndex      = -1;
        QPoint m_actionPoint;
    };

    struct Group {
        int                m_groupId = 0;
        Params             m_params;
        std::vector<Frame> m_frames;
    };

    QSize m_boundarySize;

    std::map<int, Group> m_groups;

    void load(const Mernel::std_path& jsonFilePath);
    void save(const Mernel::std_path& jsonFilePath) const;

    int               getGroupsCount() const override { return static_cast<int>(m_groups.size()); }
    QList<int>        getGroupsIds() const override;
    SpriteSequencePtr getFramesForGroup(int group) const override;
};

}
