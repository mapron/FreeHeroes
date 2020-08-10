/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

// Gui
#include "IAppSettings.hpp"
#include "ISprites.hpp"
#include "SpriteItem.hpp"  // @todo: Include not really needed so much.

// Core
#include "IBattleControl.hpp"
#include "IBattleView.hpp"
#include "IBattleNotify.hpp"

#include <QGraphicsItem>

#include <memory>


namespace FreeHeroes{
namespace Core {
struct BattleFieldGeometry;
}
namespace Sound {
class IMusicBox;
}

namespace Gui {
class ICursorLibrary;
class IAppSettings;
class BattleStackSpriteItem;
class SpriteItemObj;
class BattleControlPlan;
class AnimationSequencer;
class LibraryModelsProvider;
class GuiSpell;
using BattleStackSpriteItemPtr = BattleStackSpriteItem*;

class BattleFieldItem : public QGraphicsObject, public Core::BattleNotifyEmpty
{
    Q_OBJECT
public:
    using BattleStackConstPtr = Core::BattleStackConstPtr;
    using LibrarySpellConstPtr = Core::LibrarySpellConstPtr;
    using DamageResult = Core::DamageResult;

    BattleFieldItem(ICursorLibrary & cursorLibrary,
                    Sound::IMusicBox & musicBox,
                    LibraryModelsProvider & modelsProvider,

                    Core::IBattleView & battleView,
                    Core::IBattleControl & battleControl,
                    const Core::BattleFieldGeometry & battleGeometry,
                    BattleControlPlan & controlPlan,

                    Gui::IAppSettings & appSettings,
                    QGraphicsItem * parent = nullptr);
    ~BattleFieldItem();


    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;


    void hoverCell(Core::BattlePosition pos, const QPointF posInCell);

    void tick(int msElapsed);

    void selectCurrentStack();

 // IBattleNotifiers
    void beforeMove(BattleStackConstPtr stack, const Core::BattlePositionPath & path) override;
    void beforeAttackMelee(BattleStackConstPtr stack, BattleStackConstPtr target, DamageResult damage, bool isRetaliation) override;
    void beforeAttackRanged(BattleStackConstPtr stack, BattleStackConstPtr target, DamageResult damage) override;

    void onStackUnderEffect(BattleStackConstPtr stack, Effect effect) override;
    void onCast(const Caster & caster, const Affected & affected, LibrarySpellConstPtr spell) override;

    void onPositionReset(BattleStackConstPtr stack) override;
    void onControlAvailableChanged(bool controlAvailable) override;

    //slot
    void onSelectSpell(Core::LibrarySpellConstPtr spell);
    struct CastPresentation {
         int soundDuration = 0;
         bool useSpecialSound = false;
         const GuiSpell * spell = nullptr;
    };
    void onCastInternal(const Caster & caster, const Affected & affected, const CastPresentation & pres);

signals:
    void showInfo(BattleStackConstPtr stack, QPoint pos);
    void hideInfo();
    void heroInfoShow(bool attacker, bool defender);

private:
    void planUpdate();
    void altUpdate();
    void modifiersUpdate();
    void spellChange();

    void refreshCounters();
    void refreshHoveringState();
    void updateCursorsToPlan();
    void updateUnitHighlights();

    std::unique_ptr<AnimationSequencer> makeSequencer();

private:
    ICursorLibrary      & m_cursorLibrary;
    Sound::IMusicBox    & m_musicBox;
    LibraryModelsProvider & m_modelsProvider;

    Core::IBattleView & m_battleView;
    Core::IBattleControl & m_battleControl;
    const Core::BattleFieldGeometry & m_battleGeometry;
    BattleControlPlan & m_controlPlan;

    IAppSettings & m_appSettings;
    SporadicOrchestrator m_sporadicOrchestrator;

    struct UnitGraphics {
        BattleStackSpriteItemPtr spriteItem = nullptr;
        SpritePtr projectileSprite;
        SporadicHandle sporadic;
    };

    BattleStackSpriteItemPtr m_projectile = nullptr;
    QMap<BattleStackConstPtr, UnitGraphics> m_unitGraphics;
    Core::BattlePosition m_hovered;

    QPointF m_mousePosition;


    bool m_superspeed = false;

    bool m_showAvailableForHovered = false;

    bool m_controlAvailable = true;

    SpriteItemObj * m_projectileItem = nullptr;
    QList<SpriteItemObj *> m_stackEffects;
    QList<SpriteItemObj *> m_heroes;


};

}

}
