/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiResourceExport.hpp"

#include "Sprites.hpp"
#include "FsUtilsQt.hpp"

#include <QSet>

namespace FreeHeroes::Gui {

class GUIRESOURCE_EXPORT SpriteLoader : public Sprite
{
    QMap<int, QPair<QPixmap, QPoint>> m_frames;
    QSet<int> m_utilizedIds;

    SpriteFrame getSFrame(int id);
    using Sprite::addGroup;

public:
    void addFrame(int id, const QPixmap& data, const QPoint& padding);

    void addGroup(int groupId, const QList<int>& frameIds, const QSize & boundarySize, const SpriteSequenceParams & params);

    void addGroupSeq(int groupId, std::shared_ptr<SpriteSequence> seq);
};


struct SpriteSaveOptions {
    bool removeDuplicateFrames = true;
    bool splitIntoPngFiles = false;
};

GUIRESOURCE_EXPORT std::string makeJsonFilename(const std::string & resourceName);

GUIRESOURCE_EXPORT bool saveSprite(const SpritePtr& spriteSet, const std_path& jsonFilePath,
                const SpriteSaveOptions& options = {});

GUIRESOURCE_EXPORT SpritePtr loadSprite(const std_path& jsonFilePath);

}
