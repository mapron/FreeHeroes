/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "CursorLibrary.hpp"

namespace FreeHeroes::Gui {

CursorLibrary::CursorLibrary(IGraphicsLibrary & graphicsLibrary) {
    // crdeflt  cradvntr
    auto sprite = graphicsLibrary.getObjectAnimation("wrcombat")->get();
    auto spellCast = graphicsLibrary.getObjectAnimation("crspell")->get();
    if (!sprite || !spellCast)
        return;
    auto frames = sprite->getFramesForGroup(0);
    auto spellCastFrames = spellCast->getFramesForGroup(0);
    otherCursors[Type::Stop] = QCursor(frames->frames[0].frame, 13, 13);
    otherCursors[Type::Walk] = QCursor(frames->frames[1].frame, 12, 25);
    otherCursors[Type::Fly ] = QCursor(frames->frames[2].frame, 12, 25);
    otherCursors[Type::RangeAttack ] = QCursor(frames->frames[3].frame, 22, 3);
    otherCursors[Type::HeroView ] = QCursor(frames->frames[4].frame, 14, 12);
    otherCursors[Type::Question ] = QCursor(frames->frames[5].frame, 9, 12);
    otherCursors[Type::PlainArrow ] = QCursor(frames->frames[6].frame, 0, 0);
    otherCursors[Type::RangeAttackBroken ] = QCursor(frames->frames[15].frame, 27, 15);

    for (auto & f : spellCastFrames->frames) {
        cast <<  QCursor(f.frame, f.frame.width() / 2, f.frame.height() - 1);

    }

    QPixmap sword = frames->frames[13].frame;
    for (auto attackDirection : {
         BattleDirection::TR,
         BattleDirection::R,
         BattleDirection::BR,
         BattleDirection::BL,
         BattleDirection::L,
         BattleDirection::TL,
         BattleDirection::Top ,
         BattleDirection::Bottom ,
}) {

        int hotX = -1, hotY = -1, degree = 0;
        switch (attackDirection) {
        case BattleDirection::TR    : hotX = 22; hotY = 4 ; degree = 30 ; break;
        case BattleDirection::R     : hotX = 31; hotY = 6 ; degree = 90 ; break;
        case BattleDirection::BR    : hotX = 22; hotY = 28; degree = 150; break;
        case BattleDirection::BL    : hotX = 6 ; hotY = 31; degree = 210; break;
        case BattleDirection::L     : hotX = 0 ; hotY = 6 ; degree = 270; break;
        case BattleDirection::TL    : hotX = 6 ; hotY = 4 ; degree = 330; break;
        case BattleDirection::Top   : hotX = 6 ; hotY = 0 ; degree = 0  ; break;
        case BattleDirection::Bottom: hotX = 6 ; hotY = 31; degree = 180; break;
        default:
            break;
        }
        QCursor cur(sword.transformed(QTransform().rotate(degree)), hotX, hotY);
        attackCursors[attackDirection] = cur;
    }
}

}
