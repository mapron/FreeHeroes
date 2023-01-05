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

    struct FrameImpl {
        QPoint m_padding;
        bool   m_hasBitmap = true;

        QSize  m_bitmapSize;
        QPoint m_bitmapOffset;

        QSize m_boundarySize;
    };

    struct Group {
        int                    m_groupId = 0;
        SpriteSequenceParams   m_params;
        std::vector<FrameImpl> m_frames;

        mutable SpriteSequencePtr m_cache;
    };

    QSize m_boundarySize;

    SpriteSequenceMask m_mask;

    std::map<int, Group> m_groups;

    void load(const Mernel::std_path& jsonFilePath);
    void save(const Mernel::std_path& jsonFilePath) const;

    int               getGroupsCount() const override { return static_cast<int>(m_groups.size()); }
    QList<int>        getGroupsIds() const override;
    SpriteSequencePtr getFramesForGroup(int group) const override;
};

}
