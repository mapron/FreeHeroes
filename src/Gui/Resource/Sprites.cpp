/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "Sprites.hpp"

namespace FreeHeroes::Gui {

SpritePtr Sprite::fromPixmap(QPixmap pixmap)
{
    auto seq = std::make_shared<SpriteSequence>();
    SpriteFrame f;
    f.frame = pixmap;
    f.paddingLeftTop = {0, 0};
    seq->frames << f;

    seq->boundarySize = pixmap.size();

    Sprite result;
    result.addGroup(0, seq);
    return std::make_shared<Sprite>(std::move(result));
}

void Sprite::addGroup(int groupId, SpriteSequencePtr seq) {
    m_groups[groupId] = seq;
}

}
