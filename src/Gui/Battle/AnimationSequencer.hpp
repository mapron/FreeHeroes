/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "IMusicBox.hpp"

#include "BattleFieldPosition.hpp"
#include "BattleStack.hpp"

#include "BattleStackSpriteItem.hpp"
#include "IAppSettings.hpp"
#include "AnimationTree.hpp"

namespace FreeHeroes::Gui {

class AnimationSequencer {
public:
    enum class BattleAnimation
    {
        Move         = 0,
        Nervous      = 1,
        StandStill   = 2,
        PainRanged   = 3,
        PainMelee    = 4,
        Death        = 5,
        Death2       = 6,
        Turning      = 7, // 8,9,10 same.
        MeleeUp      = 11,
        MeleeCenter  = 12,
        MeleeDown    = 13,
        RangedUp     = 14,
        RangedCenter = 15,
        RangedDown   = 16,
        MagicUp      = 17,
        MagicCenter  = 18,
        MagicDown    = 19,
        MoveStart    = 20,
        MoveFinish   = 21,
    };
    enum class BattleSound
    {
        Move,
        PainRanged,
        PainMelee,
        Death,
        Melee,
        Ranged,
        MoveStart,
        MoveFinish,

        None
    };

    using BattlePosition         = Core::BattlePosition;
    using BattlePositionExtended = Core::BattlePositionExtended;
    using BattleDirection        = Core::BattleDirection;
    using BattleStackConstPtr    = Core::BattleStackConstPtr;
    using PosSight               = BattlePositionExtended::Sight;

public:
    class AnimationSequencerHandle {
        AnimationSequencer*      parent = nullptr;
        BattleStackSpriteItemPtr item   = nullptr;
        BattleStackConstPtr      stack  = nullptr;
        PosSight                 sight  = PosSight::ToRight;

    public:
        AnimationSequencerHandle() = default;
        AnimationSequencerHandle(AnimationSequencer* parent, BattleStackSpriteItemPtr item, BattleStackConstPtr stack);
        void changeAnim(BattleAnimation type, bool reverse = false);
        void playEffect(BattleAnimation type, int expectedDuration = -1);

        void queueChangeAnim(BattleAnimation type, int pauseDuration = -1);
        void queuePlayEffect(BattleAnimation type, int expectedDuration = -1, int pauseDuration = -1);

        bool addOptionalAnim(BattleAnimation type);

        void addPropertyAnimation(int msecs, const QByteArray& propertyName, const QVariant& endValue);
        void addPosAnimation(int msecs, QPointF endValue);
        void addPosTeleport(QPointF endValue);

        void addTurning(bool fullAnimation, bool toRight);

        bool addOptionalTurning(const BattlePosition from, const BattlePosition to, bool fullAnimation);
        bool addOptionalTurning(PosSight currentSight, bool fullAnimation);
        bool addOptionalTurningOrPause(PosSight currentSight, bool fullAnimation);

        bool addOptionalTurningToStart(bool full = true);

        int                           getAnimDuration(BattleAnimation type) const;
        SpriteItem::AnimGroupSettings getAnimSettings(BattleAnimation type) const;
    };
    AnimationSequencer(const Gui::IAppSettings::Battle& battleSettings, Sound::IMusicBox& musicBox);

    AnimationSequencerHandle* addHandle(BattleStackSpriteItemPtr item, BattleStackConstPtr stack);

    void addPropertyAnimation(QObject* target, int msecs, const QByteArray& propertyName, const QVariant& endValue);

    void addCallback(std::function<void()> callback, int pauseDuration);

    void playEffect(const std::string& soundId, int expectedDuration);
    void queuePlayEffect(const std::string& soundId, int expectedDuration, int pauseDuration);

    int                           getAnimSpeedupPercent(BattleAnimation type) const;
    SpriteItem::AnimGroupSettings getAnimSettings(BattleAnimation type) const;

    void runSync(bool excludeInput);

    void beginParallel();
    void beginSequental();
    void endGroup();
    void pause(int msec);

    void enableSuperSpeed(bool superspeed);

private:
    QList<AnimationSequencerHandle> m_handles;
    const Gui::IAppSettings::Battle m_battleSettings;
    Sound::IMusicBox&               m_musicBox;
    Gui::AnimationTree              m_tree;
    bool                            m_superspeed = false;
};

}
