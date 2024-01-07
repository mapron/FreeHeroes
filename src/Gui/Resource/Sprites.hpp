/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "ISprites.hpp"

#include "GuiResourceExport.hpp"
#include "MernelPlatform/FsUtils.hpp"

#include <map>

namespace FreeHeroes::Gui {

struct GUIRESOURCE_EXPORT Sprite : public ISprite {
    Pixmap m_bitmap;

    struct FrameImpl {
        PixmapPoint m_padding;
        bool        m_hasBitmap = true;

        PixmapSize  m_bitmapSize;
        PixmapPoint m_bitmapOffset;

        PixmapSize m_boundarySize;
    };

    struct Group {
        int                    m_groupId = 0;
        SpriteSequenceParams   m_params;
        std::vector<FrameImpl> m_frames;

        mutable SpriteSequencePtr m_cache;
    };

    PixmapSize m_boundarySize;

    SpriteSequenceMask m_mask;

    std::map<int, Group> m_groups;

    void load(const Mernel::std_path& jsonFilePath);
    void save(const Mernel::std_path& jsonFilePath) const;

    int               getGroupsCount() const override { return static_cast<int>(m_groups.size()); }
    std::vector<int>  getGroupsIds() const override;
    bool              hasGroupId(int group) const override { return m_groups.contains(group); }
    SpriteSequencePtr getFramesForGroup(int group) const override;
};

}
