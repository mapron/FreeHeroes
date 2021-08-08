/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "BattleFieldItem.hpp"

// Gui
#include "ICursorLibrary.hpp"
#include "IMusicBox.hpp"

#include "AnimationSequencer.hpp"
#include "BattleFieldPaintGeometry.hpp"
#include "BattleStackSpriteItem.hpp"
#include "LibraryModels.hpp"

// Core
#include "BattleField.hpp"
#include "BattleControlPlan.hpp"
#include "LibrarySpell.hpp"
#include "BattleHero.hpp"

#include <QGraphicsSceneEvent>
#include <QPainter>
#include <QDebug>
#include <QKeyEvent>

#include <QtMath>

namespace FreeHeroes::Core {

inline uint qHash(const BattlePosition& key, uint seed = 0)
{
    return ::qHash(QPair{ key.x, key.y }, seed);
}

}

namespace FreeHeroes::Gui {

using namespace Core;
using BattleAnimation = AnimationSequencer::BattleAnimation;

namespace {

ICursorLibrary::BattleDirection getCursorDirection(Core::BattleAttackDirection dir)
{
    // @todo: i'm lazy. need map.
    static_assert((int) Core::BattleAttackDirection::T == (int) ICursorLibrary::BattleDirection::Top);
    return static_cast<ICursorLibrary::BattleDirection>(dir);
}

qreal zValueForPosAlive(const BattlePosition& pos, bool isMoving = false)
{
    return static_cast<qreal>((pos.y + 1) * 100 + pos.x + (isMoving ? 20 : 0));
}

BattleAnimation animationForRangedDirection(BattleDirectionPrecise direction)
{
    // clang-format off
    switch(direction) {
        case BattleDirectionPrecise::Top:
        case BattleDirectionPrecise::TopTopRight:
        case BattleDirectionPrecise::TopRight:
            return BattleAnimation::RangedUp;
        case BattleDirectionPrecise::TopRightRight:
        case BattleDirectionPrecise::Right:
        case BattleDirectionPrecise::BottomRightRight:
            return BattleAnimation::RangedCenter;
        case BattleDirectionPrecise::BottomRight:
        case BattleDirectionPrecise::BottomBottomRight:
        case BattleDirectionPrecise::Bottom:
            return BattleAnimation::RangedDown;
        default:
            return BattleAnimation::RangedCenter;
    }
    // clang-format on
}

}

BattleFieldItem::BattleFieldItem(ICursorLibrary&        cursorLibrary,
                                 Sound::IMusicBox&      musicBox,
                                 LibraryModelsProvider& modelsProvider,

                                 Core::IBattleView&               battleView,
                                 Core::IBattleControl&            battleControl,
                                 const Core::BattleFieldGeometry& battleGeometry,
                                 BattleControlPlan&               controlPlan,

                                 Gui::IAppSettings& appSettings,
                                 QGraphicsItem*     parent)
    : QGraphicsObject(parent)
    , m_cursorLibrary(cursorLibrary)
    , m_musicBox(musicBox)
    , m_modelsProvider(modelsProvider)

    , m_battleView(battleView)
    , m_battleControl(battleControl)
    , m_battleGeometry(battleGeometry)
    , m_controlPlan(controlPlan)

    , m_appSettings(appSettings)

{
    m_unitGroup = new QGraphicsItemGroup(this);
    {
        // empyric offset from sprite Center point.
        const QPointF extraSpritePadding{ 0, -BattleStackSpriteItem::fromSpriteCenterToHexCenter };
        m_unitGroup->setPos(extraSpritePadding);
    }

    for (auto* stack : m_battleView.getAllStacks(false)) {
        addSpriteForBattleStack(stack);
    }

    auto makeHeroSpriteObj = [this](BattleHeroConstPtr hero, const QPointF& pos) -> SpriteItemObj* {
        auto sprite = m_modelsProvider.heroes()->find(hero->library)->getBattleSprite();
        Q_ASSERT(sprite);

        auto* heroSprite = new SpriteItemObj(this);
        heroSprite->hide();
        heroSprite->setZValue(1);
        heroSprite->setSprite(sprite);
        heroSprite->setAnimGroup(SpriteItem::AnimGroupSettings{ 1, 1500 }.setLoopOver());
        heroSprite->setPos(pos);
        heroSprite->setAcceptHoverEvents(true);
        heroSprite->setCursor(m_cursorLibrary.getOther(ICursorLibrary::Type::HeroView));
        m_heroes << heroSprite;
        return heroSprite;
    };
    auto attackerHero = m_battleView.getHero(BattleStack::Side::Attacker);
    auto defenderHero = m_battleView.getHero(BattleStack::Side::Defender);
    if (attackerHero) {
        auto* attackerHeroSprite = makeHeroSpriteObj(attackerHero, { -50, -30 });
        connect(attackerHeroSprite, &SpriteItemObj::hoverIn, this, [this] {
            emit heroInfoShow(true, false);
        });
        connect(attackerHeroSprite, &SpriteItemObj::hoverOut, this, [this] {
            emit heroInfoShow(false, false);
        });
    }
    if (defenderHero) {
        auto* defenderHeroSprite = makeHeroSpriteObj(defenderHero, { 693, -30 });
        defenderHeroSprite->setMirrorHor(true);
        connect(defenderHeroSprite, &SpriteItemObj::hoverIn, this, [this] {
            emit heroInfoShow(false, true);
        });
        connect(defenderHeroSprite, &SpriteItemObj::hoverOut, this, [this] {
            emit heroInfoShow(false, false);
        });
    }

    this->setAcceptHoverEvents(true);

    {
        const QPointF gridOffset{ 55, 83 }; // top left corner of hex grid, if extended to rectangular shape
        const QPointF firstCellOffsetFromGrid{ defaultGeometry.hexW1 + 3, defaultGeometry.hexH2 };
        const QPointF firstCellOffset = gridOffset + firstCellOffsetFromGrid;
        this->setPos(firstCellOffset);
    }
    for (auto hero : m_heroes)
        hero->show();

    m_projectileItem = new SpriteItemObj(this);
    m_projectileItem->hide();

    m_battleView.addNotify(this);

    connect(&m_controlPlan, &BattleControlPlan::planUpdated, this, &BattleFieldItem::planUpdate);
    connect(&m_controlPlan, &BattleControlPlan::modifiersChanged, this, &BattleFieldItem::modifiersUpdate);
    connect(&m_controlPlan, &BattleControlPlan::altUpdated, this, &BattleFieldItem::altUpdate);

    refreshCounters();
}

BattleFieldItem::~BattleFieldItem() = default;

QRectF BattleFieldItem::boundingRect() const
{
    //return QRectF{-0, 0, 800, 556};
    return QRectF{ -100, -100, 800 + 100, 556 + 100 };
}

void BattleFieldItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    const QBrush shadowAvailable             = QColor(0, 0, 0, 90);
    const QBrush shadowMoveTarget            = QColor(0, 0, 0, 170);
    const QBrush hightlightFocus             = QColor(200, 200, 200, 40);
    const QBrush hightlightAttackFocus       = QColor(50, 0, 0, 150);
    const QBrush hightlightRetaliationSplash = QColor(150, 100, 0, 150);
    const auto   mainGrid                    = QColor(192, 192, 0, 80);
    const auto   hintGrid                    = QColor(0, 0, 0, 100);
    const auto   pathTrace                   = QColor(0, 150, 0, 150);
    const QBrush obstacleColor               = QColor(60, 0, 0, 120);

    auto drawCellBorder = [&painter](BattlePosition pos, qreal scale) {
        painter->drawPolygon(defaultGeometry.getHexPolygon(pos, scale));
    };
    painter->setBrush(QBrush(Qt::NoBrush));
    painter->setPen(m_appSettings.battle().displayGrid ? QPen(mainGrid, 0.0) : Qt::NoPen);

    auto getAvailableMovement = [this](BattleStackConstPtr stack) -> QSet<BattlePosition> {
        if (!stack)
            return {};

        QSet<BattlePosition> result;
        auto                 avai = m_battleView.findAvailable(stack);
        for (auto el : avai)
            result << el;
        return result;
    };
    auto getAvailableCastArea = [this]() -> QSet<BattlePosition> {
        QSet<BattlePosition> result;
        for (auto el : m_controlPlan.m_planCast.m_affectedArea)
            result << el;
        return result;
    };
    auto getRetaliationSplashArea = [this]() -> QSet<BattlePosition> {
        QSet<BattlePosition> result;
        for (auto target : m_controlPlan.m_planMove.m_extraRetaliationAffectedTargets) {
            result << target.stack->pos.leftPos();
            result << target.stack->pos.rightPos();
        }
        return result;
    };

    const bool validMove             = m_controlPlan.m_planMove.isValid();
    const bool makingCast            = m_controlPlan.m_planCastParams.isActive();
    const int  checkAvaiDx           = m_controlPlan.m_selectedStack && m_controlPlan.m_selectedStack->library->traits.large ? (m_controlPlan.m_selectedStack->side == BattleStack::Side::Attacker ? -1 : +1) : 0;
    const auto currentAvailableCells = makingCast ? getAvailableCastArea() : getAvailableMovement(m_controlPlan.m_selectedStack);
    const auto hoveredAvailableCells = m_showAvailableForHovered ? getAvailableMovement(m_controlPlan.m_hoveredStack) : QSet<BattlePosition>{};
    const auto retaliationSplashArea = validMove ? getRetaliationSplashArea() : QSet<BattlePosition>{};

    QSet<BattlePosition> obstacles;
    for (auto pos : m_battleView.getObstacles())
        obstacles << pos;

    for (int h = 0, totalH = m_battleGeometry.height; h < totalH; ++h) {
        for (int w = 0, totalW = m_battleGeometry.width; w < totalW; ++w) {
            const BattlePosition pos{ w, h };
            const bool           isHovered         = m_hovered == pos;
            const bool           isHoveredOnAttack = !makingCast && validMove && ((isHovered && m_controlPlan.m_hoveredStack) || m_controlPlan.m_planMove.m_splashPositions.contains(pos));
            const bool           isMoveTo          = validMove && m_controlPlan.m_planMove.m_moveTo.contains(pos);
            QBrush               brush             = QBrush(Qt::NoBrush);
            if (isMoveTo)
                brush = shadowMoveTarget;
            else if (obstacles.contains(pos))
                brush = obstacleColor;
            else if (isHoveredOnAttack)
                brush = hightlightAttackFocus;
            else if (isHovered && !makingCast)
                brush = hightlightFocus;
            else if (retaliationSplashArea.contains(pos))
                brush = hightlightRetaliationSplash;
            else if (currentAvailableCells.contains(pos))
                brush = shadowAvailable;
            else if (!makingCast && currentAvailableCells.contains({ pos.x + checkAvaiDx, pos.y }))
                brush = shadowAvailable;

            painter->setBrush(brush);
            drawCellBorder(pos, m_appSettings.battle().displayGrid ? 1. : 0.95);
        }
    }

    painter->setBrush(QBrush(Qt::NoBrush));
    painter->setPen(QPen(hintGrid, 3.));
    for (const auto pos : hoveredAvailableCells) {
        drawCellBorder(pos, 0.7);
    }

    if (m_appSettings.battle().displayPath) {
        painter->setPen(QPen(pathTrace, 3, Qt::DotLine));
        if (m_controlPlan.m_planMove.isValid() && !m_controlPlan.m_planMove.m_walkPath.empty()) {
            QVector<QLineF> lines;
            for (size_t i = 0; i < m_controlPlan.m_planMove.m_walkPath.size(); ++i) {
                const BattlePosition from = i > 0 ? m_controlPlan.m_planMove.m_walkPath[i - 1] : m_controlPlan.m_planMove.m_moveFrom.mainPos();
                const BattlePosition to   = m_controlPlan.m_planMove.m_walkPath[i];

                const auto fromCenter = defaultGeometry.hexCenterFromCoord(from);
                const auto toCenter   = defaultGeometry.hexCenterFromCoord(to);

                lines << QLineF{ fromCenter, toCenter };
            }
            painter->drawLines(lines);
        }
    }
}

void BattleFieldItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    m_mousePosition = event->pos();

    m_controlPlan.setModifiers(event->modifiers());
    refreshHoveringState();
}

void BattleFieldItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    m_mousePosition = event->pos();
    auto battlePos  = defaultGeometry.coordFromPos(m_mousePosition);
    if (!m_battleGeometry.isValid(battlePos))
        return;

    if (event->button() == Qt::LeftButton) {
        if (m_controlPlan.m_planCastParams.isActive()) {
            if (m_controlPlan.m_planCast.isValid()) {
                auto copyParams = m_controlPlan.m_planCastParams;
                m_controlPlan.m_planCastParams.clear();
                m_battleControl.doCast(copyParams);
                return;
            }
            return;
        }
        if (m_controlPlan.m_planAttackParams.isActive() || m_controlPlan.m_planMoveParams.isActive()) {
            if (m_controlPlan.m_planMove.isValid()) {
                m_controlPlan.m_selectedStack = nullptr;
                auto copyMove                 = m_controlPlan.m_planMoveParams;
                auto copyAttack               = m_controlPlan.m_planAttackParams;
                m_controlPlan.m_planMoveParams.clear();
                m_controlPlan.m_planAttackParams.clear();
                if (!copyAttack.isActive())
                    m_battleControl.doMoveAttack(copyMove, {});
                else
                    m_battleControl.doMoveAttack(copyMove, copyAttack);

                return;
            }
            return;
        }
    } else if (event->button() == Qt::RightButton) {
        if (m_controlPlan.m_planCastParams.m_isHeroCast) {
            m_controlPlan.setHeroSpell(nullptr);
            m_controlPlan.sendPlanUpdated();
            return;
        }
        emit showInfo(m_battleView.findStack(battlePos, true), m_mousePosition.toPoint());
    }
}

void BattleFieldItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    emit hideInfo();
    QGraphicsObject::mouseReleaseEvent(event);
}

void BattleFieldItem::hoverCell(BattlePosition battlePos, const QPointF posInCell)
{
    // possible variants:
    // 1. hover reachable empty cell and plan move there
    // 2. hover unreachable empty cell - do nothing
    // 3. hover unreachable enemy or  own stack - show hoverHint
    // 4. hover reachable enemy - set cursor as attack arrow, create suggested move plan
    if (!m_controlPlan.m_selectedStack) {
        return;
    }

    bool onlyAlive               = true; // @todo: on resurrect maybe changed
    m_controlPlan.m_hoveredStack = m_battleView.findStack(battlePos, onlyAlive);

    if (m_controlPlan.m_hoveredStack) {
        if (m_controlPlan.m_selectedStack != m_controlPlan.m_hoveredStack) {
            m_showAvailableForHovered = m_controlPlan.currentModifiers().testFlag(Qt::ControlModifier);
        }
    }

    if (!m_controlPlan.m_humanControlAvailable)
        return;

    if (m_controlPlan.m_planCastParams.isActive()) {
        m_controlPlan.m_planCastParams.m_target = battlePos;
        m_controlPlan.m_planCast                = m_battleView.findPlanCast(m_controlPlan.m_planCastParams);
        return;
    }
    m_controlPlan.m_planMoveParams.m_moveFrom     = m_controlPlan.m_selectedStack->pos;
    const bool currentStackIsWide                 = m_controlPlan.m_selectedStack->pos.isLarge();
    m_controlPlan.m_planAttackParams.m_alteration = m_controlPlan.getAlt();
    if (m_controlPlan.m_hoveredStack || m_controlPlan.m_planAttackParams.m_alteration == BattlePlanAttackParams::Alteration::FreeAttack) {
        m_controlPlan.m_planAttackParams.m_attackDirection = defaultGeometry.attackDirectionFromRelativePoint(posInCell, currentStackIsWide);
        m_controlPlan.m_planAttackParams.m_attackTarget    = battlePos;
        m_controlPlan.m_planMoveParams.m_movePos           = m_controlPlan.m_selectedStack->pos;
        if (m_controlPlan.m_planAttackParams.m_alteration != BattlePlanAttackParams::Alteration::FreeAttack)
            m_controlPlan.m_planMoveParams.m_movePos = m_battleGeometry.suggestPositionForAttack(m_controlPlan.m_selectedStack->pos,
                                                                                                 m_controlPlan.m_hoveredStack->pos,
                                                                                                 m_controlPlan.m_hoveredStack->pos.mainPos() == battlePos ? BattlePositionExtended::Sub::Main : BattlePositionExtended::Sub::Secondary,
                                                                                                 m_controlPlan.m_planAttackParams.m_attackDirection);
        m_controlPlan.m_planMove = m_battleView.findPlanMove(m_controlPlan.m_planMoveParams, m_controlPlan.m_planAttackParams);
        if (m_controlPlan.m_planMove.isValid()) {
            //assert(m_battleGeometry.isValid(m_controlPlan.m_planMoveParams.m_movePos.mainPos()));
            //assert(m_battleGeometry.isValid(m_controlPlan.m_planAttackParams.m_attackTargetPos.mainPos()));
        }
    } else {
        m_controlPlan.m_planMoveParams.m_movePos = m_controlPlan.m_selectedStack->pos.moveMainTo(battlePos);
        m_controlPlan.m_planMove                 = m_battleView.findPlanMove(m_controlPlan.m_planMoveParams, {});
        if (!m_controlPlan.m_planMove.isValid() && currentStackIsWide) {
            BattlePosition altPos    = { battlePos.x + (m_controlPlan.m_selectedStack->pos.sightDirectionIsRight() ? -1 : +1), battlePos.y };
            auto           altParams = m_controlPlan.m_planMoveParams;
            altParams.m_movePos      = m_controlPlan.m_selectedStack->pos.moveMainTo(altPos);
            auto planAlt             = m_battleView.findPlanMove(altParams, {});
            if (planAlt.isValid()) {
                m_controlPlan.m_planMoveParams = altParams;
                m_controlPlan.m_planMove       = planAlt;
            }
        }

        if (m_controlPlan.m_planMove.isValid()) {
            assert(m_battleGeometry.isValid(m_controlPlan.m_planMoveParams.m_movePos.mainPos()));
        }
    }
}

void BattleFieldItem::tick(int msElapsed)
{
    for (auto& item : m_unitGraphics) {
        if (!item.animationEnabled)
            continue;
        item.sporadic.tick(msElapsed);
        item.spriteItem->tick(msElapsed);
    }
    for (auto* stackEffect : m_stackEffects) {
        if (stackEffect->isVisible())
            stackEffect->tick(msElapsed);
    }
    for (auto* hero : m_heroes) {
        hero->tick(msElapsed);
    }
}

void BattleFieldItem::selectCurrentStack()
{
}

void BattleFieldItem::beforeMove(BattleStackConstPtr stack, const BattlePositionPath& path)
{
    if (path.empty())
        return;

    Q_ASSERT(stack->isAlive());

    const BattlePosition   start  = stack->pos.mainPos();
    BattlePositionExtended tmpPos = stack->pos;

    const bool             canFly      = stack->library->traits.fly;
    const bool             canTeleport = stack->library->traits.teleport;
    BattleStackSpriteItem* item        = m_unitGraphics[stack].spriteItem;

    item->setCounterVisible(false);

    auto  sequencer = makeSequencer();
    auto* seqHandle = sequencer->addHandle(item, stack);

    if (canFly || canTeleport) {
        seqHandle->addOptionalTurning(start, path.back(), true);
    }

    sequencer->beginParallel();
    if (seqHandle->addOptionalAnim(BattleAnimation::MoveStart))
        seqHandle->queuePlayEffect(BattleAnimation::MoveStart, seqHandle->getAnimDuration(BattleAnimation::MoveStart), 1);
    sequencer->endGroup();

    if (!canTeleport)
        seqHandle->queueChangeAnim(BattleAnimation::Move, 1);

    const int moveDurationOne   = seqHandle->getAnimDuration(BattleAnimation::Move);
    const int moveDurationTotal = canTeleport ? 0 : (canFly ? moveDurationOne * path.size() / 3 : moveDurationOne * path.size());

    seqHandle->queuePlayEffect(BattleAnimation::Move, moveDurationTotal, 1);

    // qDebug() << canFly << canTeleport;

    if (canFly || canTeleport) {
        tmpPos.setMainPos(path.back());
        const auto dest = defaultGeometry.hexCenterFromExtCoord(tmpPos);
        if (canTeleport)
            seqHandle->addPosTeleport(dest);
        else
            seqHandle->addPosAnimation(moveDurationTotal, dest);
        auto endZvalue = zValueForPosAlive(path.back());
        if (endZvalue < item->zValue() && canFly)
            seqHandle->addPropertyAnimation(1, "zValue", endZvalue);
        else if (canFly)
            item->setZValue(endZvalue);
    } else {
        auto prevPos = start;
        bool useFull = true;
        for (auto pos : path) {
            if (seqHandle->addOptionalTurning(prevPos, pos, useFull))
                seqHandle->queueChangeAnim(BattleAnimation::Move, 1);
            useFull = false;

            const auto prevZvalue = zValueForPosAlive(prevPos, true);
            prevPos               = pos;
            const auto endZvalue  = zValueForPosAlive(pos, true);

            if (prevZvalue > endZvalue) // from nearest to  distant
                seqHandle->addPropertyAnimation(1, "zValue", endZvalue);

            tmpPos.setMainPos(pos);
            seqHandle->addPosAnimation(moveDurationOne, defaultGeometry.hexCenterFromExtCoord(tmpPos));
            if (prevZvalue < endZvalue) // from distant to nearest
                seqHandle->addPropertyAnimation(1, "zValue", endZvalue);
        }
    }
    seqHandle->addPropertyAnimation(1, "zValue", zValueForPosAlive(path.back(), false));
    sequencer->beginParallel();
    if (seqHandle->addOptionalAnim(BattleAnimation::MoveFinish))
        seqHandle->queuePlayEffect(BattleAnimation::MoveFinish, seqHandle->getAnimDuration(BattleAnimation::MoveFinish), 1);
    sequencer->endGroup();

    sequencer->runSync(false);

    seqHandle->changeAnim(BattleAnimation::StandStill);

    refreshCounters();
}

void BattleFieldItem::beforeAttackMelee(BattleStackConstPtr stack, const AffectedPhysical& affected, bool isRetaliation)
{
    (void) isRetaliation;
    auto target = affected.main.stack;
    enableAnimationFor(affected);
    BattleStackSpriteItem* itemAttacker    = m_unitGraphics[stack].spriteItem;
    m_unitGraphics[stack].animationEnabled = true;

    Q_ASSERT(stack->isAlive());
    Q_ASSERT(target->isAlive());

    auto  sequencer = makeSequencer();
    auto* attHandle = sequencer->addHandle(itemAttacker, stack);

    sequencer->beginParallel();

    auto animType = BattleAnimation::MeleeCenter;
    if (stack->pos.mainPos().y < target->pos.mainPos().y)
        animType = BattleAnimation::MeleeDown;
    else if (stack->pos.mainPos().y > target->pos.mainPos().y)
        animType = BattleAnimation::MeleeUp;

    const bool isTurningForAttack  = stack->pos.isInversePositionRelatedTo(target->pos);
    const int  attackStartDuration = attHandle->getAnimDuration(animType) * 4 / 5;
    const int  attackEndDuration   = attHandle->getAnimDuration(animType) / 5;

    {
        sequencer->beginSequental();
        {
            attHandle->addOptionalTurningOrPause(isTurningForAttack ? stack->pos.inverseSightDirection() : stack->pos.sightDirection(), true);
            sequencer->beginParallel();
            {
                attHandle->queueChangeAnim(animType);
                attHandle->queuePlayEffect(animType, attackStartDuration, 1);
            }
            sequencer->endGroup(); // parallel
            attHandle->queueChangeAnim(BattleAnimation::StandStill, 1);
        }
        sequencer->endGroup(); // sequental
    }
    BattleStackSpriteItem* mainDefender  = m_unitGraphics[target].spriteItem;
    auto*                  defMainHandle = sequencer->addHandle(mainDefender, target);
    for (const auto& affectedTarget : affected.getAll()) {
        BattleStackSpriteItem* itemDefender = m_unitGraphics[affectedTarget.stack].spriteItem;
        const auto&            damage       = affectedTarget.damage;
        auto*                  defHandle    = (affectedTarget.stack == target) ? defMainHandle : sequencer->addHandle(itemDefender, affectedTarget.stack);
        const auto             defAnim      = damage.isKilled() ? BattleAnimation::Death : BattleAnimation::PainMelee;

        sequencer->beginSequental();
        {
            if (affectedTarget.stack == target)
                defHandle->addOptionalTurningOrPause(isTurningForAttack ? target->pos.inverseSightDirection() : target->pos.sightDirection(), true);
            else
                sequencer->pause(defMainHandle->getAnimDuration(BattleAnimation::Turning));
            defHandle->queueChangeAnim(BattleAnimation::StandStill, 1);
            sequencer->pause(attackStartDuration);
            sequencer->beginParallel();
            {
                defHandle->queueChangeAnim(defAnim);
                defHandle->queuePlayEffect(defAnim, attackEndDuration, 1);
            }
            sequencer->endGroup(); // parallel
            if (!damage.isKilled()) {
                defHandle->queueChangeAnim(BattleAnimation::StandStill, 1);
            } else {
                defHandle->addDeathPropertyAnimations(damage.loss.permanent);
            }
        }
        sequencer->endGroup(); // sequental
    }

    sequencer->endGroup(); // parallel

    sequencer->runSync(false);

    refreshCounters();
}

void BattleFieldItem::beforeAttackRanged(BattleStackConstPtr stack, const AffectedPhysical& affected)
{
    BattleStackSpriteItem* itemAttacker = m_unitGraphics[stack].spriteItem;
    enableAnimationFor(affected);

    SpritePtr projectileSprite = m_unitGraphics[stack].projectileSprite;

    auto  sequencer = makeSequencer();
    auto* attHandle = sequencer->addHandle(itemAttacker, stack);

    const GuiSpell* splashSpell = nullptr;
    if (stack->library->abilities.splashSpell)
        splashSpell = m_modelsProvider.spells()->find(stack->library->abilities.splashSpell);

    SpriteItemObj* splashItem = nullptr;

    if (splashSpell) {
        auto sprite = splashSpell->getAnimation();
        splashItem  = new SpriteItemObj(this);
        splashItem->hide();
        m_stackEffects << splashItem;
        splashItem->setSprite(sprite);
    }

    sequencer->beginParallel();

    const auto             nearest              = stack->pos.shortestDecartDistanceSqr(affected.mainTargetPos);
    const int              approxDecartDistance = static_cast<int>(std::sqrt(static_cast<double>(nearest.first)) / BattlePosition::decartMultiplier);
    const BattlePosition   startPos             = nearest.second.first;
    const BattlePosition   targetPos            = nearest.second.second;
    BattleDirectionPrecise directionTo          = startPos.preciseDirectionTo(targetPos);
    const bool             attackDirectionLeft  = directionTo > BattleDirectionPrecise::Bottom;
    if (attackDirectionLeft)
        directionTo = directionMirrorHor(directionTo);
    const auto animType = animationForRangedDirection(directionTo);

    // const bool needTurningForAttack = mirroredDirectionTo;
    const bool turningAnimationRequired  = (itemAttacker->getTmpDirectionRight() && attackDirectionLeft) || (!itemAttacker->getTmpDirectionRight() && !attackDirectionLeft);
    const int  attackNormalDuration      = attHandle->getAnimDuration(animType);
    const auto frameGroup                = itemAttacker->getSprite()->getFramesForGroup(static_cast<int>(animType));
    const int  attackFramesSize          = frameGroup->frames.size();
    const int  climaxFrameIndex          = std::max(frameGroup->params.specialFrameIndex - 1, 0);
    const int  climaxFrameRepeatCount    = projectileSprite ? approxDecartDistance / 2 : 0;
    const int  climaxFrameRepeatDuration = attackNormalDuration * climaxFrameRepeatCount / attackFramesSize;
    const int  beforeClimaxDuration      = attackNormalDuration * climaxFrameIndex / attackFramesSize;
    const int  attackFullDuration        = attackNormalDuration + climaxFrameRepeatDuration;
    const int  turningAnimationDuration  = turningAnimationRequired ? attHandle->getAnimDuration(BattleAnimation::Turning) : 0;
    {
        //const QString audioId = QString::fromStdString(stack->library->pres.soundId) + (stack->library->pres.soundHasShoot ? "shot" : "attk");
        sequencer->beginSequental();
        {
            if (turningAnimationRequired)
                attHandle->addTurning(true, !attackDirectionLeft);

            sequencer->beginParallel();
            {
                sequencer->addCallback([itemAttacker, animType, attackNormalDuration, climaxFrameIndex, climaxFrameRepeatCount]() {
                    itemAttacker->setAnimGroup(SpriteItem::AnimGroupSettings{ static_cast<int>(animType), attackNormalDuration }
                                                   .setRepeatFrame(climaxFrameIndex, climaxFrameRepeatCount));
                },
                                       attackFullDuration);
                attHandle->queuePlayEffect(animType, attackNormalDuration, 1);
            }
            sequencer->endGroup(); // parallel
            attHandle->queueChangeAnim(BattleAnimation::StandStill, 1);
        }
        sequencer->endGroup(); // sequental
    }

    {
        int maxDefDuration = 1;
        for (const auto& affectedTarget : affected.getAll()) {
            BattleStackSpriteItem* itemDefender = m_unitGraphics[affectedTarget.stack].spriteItem;
            auto*                  defHandle    = sequencer->addHandle(itemDefender, affectedTarget.stack);
            const auto&            damage       = affectedTarget.damage;
            const auto             animationDef = damage.isKilled() ? BattleAnimation::Death : BattleAnimation::PainRanged;
            const auto             defDuration  = defHandle->getAnimDuration(animationDef);
            maxDefDuration                      = std::max(maxDefDuration, defDuration);

            sequencer->beginSequental();
            {
                sequencer->pause(turningAnimationDuration + beforeClimaxDuration + climaxFrameRepeatDuration);
                sequencer->beginParallel();
                {
                    defHandle->queueChangeAnim(animationDef);
                    defHandle->queuePlayEffect(animationDef, defDuration, 1);
                }
                sequencer->endGroup(); // parallel
                if (!damage.isKilled()) {
                    defHandle->queueChangeAnim(BattleAnimation::StandStill, 1);
                } else {
                    defHandle->addDeathPropertyAnimations(damage.loss.permanent);
                }
            }
            sequencer->endGroup(); // sequental
        }

        if (splashItem) {
            auto soundEffect = splashSpell->getSound();
            splashItem->setAnimGroup(SpriteItem::AnimGroupSettings{ 0, maxDefDuration }.setLoopOver(false));
            auto posGlobal = defaultGeometry.hexCenterFromCoord(targetPos);
            splashItem->setPos(posGlobal - QPoint(0, 0));
            sequencer->beginSequental();
            {
                sequencer->pause(turningAnimationDuration + beforeClimaxDuration + climaxFrameRepeatDuration);
                sequencer->addCallback([soundEffect, maxDefDuration]() {
                    soundEffect->playFor(maxDefDuration + 500);
                },
                                       1);
                sequencer->addCallback([this]() {
                    for (auto* effect : m_stackEffects)
                        effect->show();
                },
                                       maxDefDuration);
                sequencer->addCallback([this]() {
                    for (auto* effect : m_stackEffects) {
                        effect->hide();
                        effect->deleteLater();
                    }
                    m_stackEffects.clear();
                },
                                       1);
            }
            sequencer->endGroup(); // sequental
        }
    }

    if (projectileSprite) {
        const auto   projOffsets   = frameGroup->params.actionPoint;
        QPoint       initialOffset = projOffsets.isNull() ? QPoint{ 40, -60 } : projOffsets;
        const QPoint directionOffset{ attackDirectionLeft ? -30 : 20, 20 };
        if (attackDirectionLeft)
            initialOffset.setX(-initialOffset.x());

        int spriteFrame = 0;
        if (projectileSprite->getFramesForGroup(0)->frames.size() == 9)
            spriteFrame = static_cast<int>(directionTo);

        m_projectileItem->setSprite(projectileSprite);
        m_projectileItem->setAnimGroup(SpriteItem::AnimGroupSettings{ 0, 1 }.setStartFrame(spriteFrame));
        auto projectilePosStart = defaultGeometry.hexCenterFromExtCoord(stack->pos);
        auto projectilePosEnd   = defaultGeometry.hexCenterFromCoord(targetPos);
        m_projectileItem->setMirrorHor(attackDirectionLeft);
        m_projectileItem->setPos(projectilePosStart + initialOffset + directionOffset);

        sequencer->beginSequental();
        {
            sequencer->pause(turningAnimationDuration + beforeClimaxDuration);
            sequencer->addCallback([this]() {
                m_projectileItem->show();
            },
                                   1);

            sequencer->addPropertyAnimation(m_projectileItem, climaxFrameRepeatDuration, "pos", projectilePosEnd);
            sequencer->addCallback([this]() {
                m_projectileItem->hide();
            },
                                   1);
        }
        sequencer->endGroup(); // sequental
    }

    sequencer->endGroup(); // parallel

    sequencer->runSync(false);

    refreshCounters();
}

void BattleFieldItem::onStackUnderEffect(BattleStackConstPtr stack, Effect effect)
{
    // clang-format off
    const std::vector<std::string> res { // GoodMorale, BadMorale, GoodLuck, BadLuck, Resist, Regenerate
       "sod.effect.goodMorale"   ,
       "sod.effect.badMorale"    ,
       "sod.effect.goodLuck"     ,
       "sod.effect.badLuck"      ,
       "sod.effect.resist"       ,
       "sod.effect.regen"        ,
    };
    // clang-format on
    const QList<int> durations{ 1800, 2300, 3500, 2600 };
    const int        index = static_cast<int>(effect);
    AffectedMagic    affected;
    affected.targets.push_back({ stack, {} });
    affected.area.push_back(stack->pos.mainPos());
    CastPresentation pres;

    pres.spell         = m_modelsProvider.spells()->find(res[index]);
    pres.soundDuration = durations.value(index, 1800);

    onCastInternal({}, affected, pres);
}

void BattleFieldItem::onCast(const Caster& caster, const AffectedMagic& affected, BattleFieldItem::LibrarySpellConstPtr spell)
{
    const int        soundDuration = 1500; // @todo: ?
    CastPresentation pres;
    pres.soundDuration = soundDuration;
    pres.spell         = m_modelsProvider.spells()->find(spell);
    onCastInternal(caster, affected, pres);
    m_controlPlan.m_planCast = {};
}

void BattleFieldItem::onSummon(const Caster&, LibrarySpellConstPtr spell, BattleStackConstPtr stack)
{
    addSpriteForBattleStack(stack);
    BattleStackSpriteItem* item = m_unitGraphics[stack].spriteItem;
    item->setOpacity(0);

    auto* guiSpell = m_modelsProvider.spells()->find(spell);

    const int animationDuration       = std::max(1, 1500 * m_appSettings.battle().otherTimePercent / 100);
    const int animationDurationExtend = animationDuration + 2000;
    const int soundDurationMax        = std::min(1500, animationDurationExtend);

    auto  sequencer   = makeSequencer();
    auto* itemHandle  = sequencer->addHandle(item, stack);
    auto  soundHandle = guiSpell->getSound();
    sequencer->beginParallel();
    {
        sequencer->addCallback([soundHandle, soundDurationMax]() {
            soundHandle->playFor(soundDurationMax);
        },
                               1);
        itemHandle->addPropertyAnimation(animationDuration, "opacity", 1.);
    }
    this->update();
    sequencer->runSync(false);
    refreshCounters();
}

void BattleFieldItem::onPositionReset(BattleStackConstPtr stack)
{
    auto                   sequencer = makeSequencer();
    BattleStackSpriteItem* item      = m_unitGraphics[stack].spriteItem;
    auto*                  seqHandle = sequencer->addHandle(item, stack);
    if (stack->count > 0 && seqHandle->addOptionalTurningToStart())
        sequencer->runSync(false);
}

void BattleFieldItem::onControlAvailableChanged(bool controlAvailable)
{
    if (!controlAvailable) {
        m_controlPlan.m_selectedStack = nullptr;
        m_controlPlan.m_hoveredStack  = nullptr;
        m_controlPlan.m_planMove.clear();
        m_controlPlan.m_planCastParams.clear();
    }
    if (controlAvailable) {
        m_controlPlan.m_selectedStack = m_battleView.getActiveStack();
        //        AnimationSequencer seq(m_uiSettings, *m_musicBox);
        //        BattleStackSpriteItem * item =  m_unitGraphics[stack].spriteItem;
        //        seq.addHandle(item, stack)->changeAnim(BattleAnimation::StandStill);
    }
    m_controlAvailable = controlAvailable;
    updateUnitHighlights();
    refreshCounters();
    this->update();
}

void BattleFieldItem::onCastInternal(const Caster& caster, const AffectedMagic& affected, const CastPresentation& pres)
{
    auto sequencer = makeSequencer();
    (void) caster;

    const int animationDuration       = std::max(1, 1500 * m_appSettings.battle().otherTimePercent / 100);
    const int animationDurationExtend = animationDuration + 2000;
    const int soundDurationMax        = std::min(pres.soundDuration, animationDurationExtend);

    bool animationOnMainPosition = pres.spell->getSource()->presentationParams.animationOnMainPosition;
    for (const auto& target : affected.targets)
        m_unitGraphics[target.stack].animationEnabled = true;

    QList<SpriteItemObj*> extraBottom;
    if (!animationOnMainPosition) {
        for (const auto& target : affected.targets) {
            auto posGlobal = defaultGeometry.hexCenterFromExtCoord(target.stack->pos);
            if (pres.spell->hasBottomAnimation()) {
                auto sprite = pres.spell->getBottomAnimation();
                Q_ASSERT(sprite);
                auto* stackEffect = new SpriteItemObj(this);
                stackEffect->hide();

                stackEffect->setDrawOriginV(SpriteItem::DrawOriginV::Bottom);

                auto effectPos = posGlobal + QPoint(0, defaultGeometry.hexH1);
                stackEffect->setPos(effectPos);

                stackEffect->setSprite(sprite);
                stackEffect->setAnimGroup(SpriteItem::AnimGroupSettings{ 0, animationDuration });
                m_stackEffects << stackEffect;
                extraBottom << stackEffect;
            }

            if (!pres.spell->getSource()->presentationParams.animation.empty()) {
                auto sprite = pres.spell->getAnimation();
                if (sprite) {
                    auto* stackEffect = new SpriteItemObj(this);
                    stackEffect->hide();

                    auto effectPos = posGlobal - QPoint(0, defaultGeometry.hexH1 * 2);
                    if (pres.spell->getSource()->presentationParams.bottomPosition) {
                        stackEffect->setDrawOriginV(SpriteItem::DrawOriginV::Bottom);
                        effectPos = posGlobal + QPoint(0, defaultGeometry.hexH1);
                    }
                    stackEffect->setPos(effectPos);

                    m_stackEffects << stackEffect;

                    stackEffect->setSprite(sprite);
                    stackEffect->setAnimGroup(SpriteItem::AnimGroupSettings{ 0, animationDuration }.setLoopOver(false));
                }
            }
        }
    } else {
        auto posGlobal = defaultGeometry.hexCenterFromCoord(affected.mainPosition);
        auto sprite    = pres.spell->getAnimation();
        if (sprite) {
            auto* stackEffect = new SpriteItemObj(this);
            stackEffect->hide();

            auto effectPos = posGlobal - QPoint(0, defaultGeometry.hexH1 * 2);
            if (pres.spell->getSource()->presentationParams.bottomPosition) {
                stackEffect->setDrawOriginV(SpriteItem::DrawOriginV::Bottom);
                effectPos = posGlobal + QPoint(0, defaultGeometry.hexH1);
            }
            stackEffect->setPos(effectPos);

            m_stackEffects << stackEffect;

            stackEffect->setSprite(sprite);
            stackEffect->setAnimGroup(SpriteItem::AnimGroupSettings{ 0, animationDuration }.setLoopOver(false));
        }
    }

    auto soundHandle = pres.spell->getSound();
    sequencer->beginParallel();
    {
        sequencer->addCallback([soundHandle, soundDurationMax]() {
            soundHandle->playFor(soundDurationMax);
        },
                               1);

        if (!extraBottom.isEmpty()) {
            sequencer->beginSequental();
            {
                sequencer->pause(animationDuration / 5);
                sequencer->addCallback([extraBottom]() {
                    for (auto* effect : extraBottom)
                        effect->hide();
                },
                                       1);
            }
            sequencer->endGroup();
        }

        sequencer->beginSequental();
        {
            sequencer->addCallback([this]() {
                for (auto* effect : m_stackEffects)
                    effect->show();
            },
                                   animationDuration);
            sequencer->addCallback([this]() {
                for (auto* effect : m_stackEffects) {
                    effect->hide();
                    effect->deleteLater();
                }
                m_stackEffects.clear();
            },
                                   1);
        }
        sequencer->endGroup();

        sequencer->beginParallel();
        for (const auto& target : affected.targets) {
            if (target.loss.damageTotal == 0 || (target.loss.deaths < 0 && target.loss.remainCount != -target.loss.deaths))
                continue;

            auto                   affectedBattleStack = target.stack;
            BattleStackSpriteItem* itemDefender        = m_unitGraphics[affectedBattleStack].spriteItem;
            auto*                  defHandle           = sequencer->addHandle(itemDefender, affectedBattleStack);
            const bool             isKilled            = target.loss.remainCount == 0;
            const bool             isResurrect         = target.loss.deaths < 0;
            const auto             animationDef        = isResurrect ? BattleAnimation::Death : (isKilled ? BattleAnimation::Death : BattleAnimation::PainRanged);
            const auto             defDuration         = defHandle->getAnimDuration(animationDef);
            sequencer->beginSequental();
            {
                sequencer->pause(animationDuration / 2);
                sequencer->beginParallel();
                {
                    defHandle->queueChangeAnim(animationDef, -1, isResurrect);
                    if (!isResurrect)
                        defHandle->queuePlayEffect(animationDef, defDuration, 1);
                }
                sequencer->endGroup(); // parallel
                if (!isKilled) {
                    defHandle->queueChangeAnim(BattleAnimation::StandStill, 1);
                } else {
                    defHandle->addDeathPropertyAnimations(target.loss.permanent);
                }
            }
            sequencer->endGroup(); // sequental
        }

        sequencer->endGroup();
    }
    sequencer->endGroup();

    this->update();
    sequencer->runSync(false);
    refreshCounters();
}

void BattleFieldItem::enableAnimationFor(const IBattleNotify::AffectedPhysical& affected)
{
    for (const auto& target : affected.extra)
        m_unitGraphics[target.stack].animationEnabled = true;
    if (affected.main.stack)
        m_unitGraphics[affected.main.stack].animationEnabled = true;
}

void BattleFieldItem::planUpdate()
{
}

void BattleFieldItem::altUpdate()
{
    refreshHoveringState();
}

void BattleFieldItem::modifiersUpdate()
{
    m_superspeed = m_controlPlan.currentModifiers().testFlag(Qt::ShiftModifier);
    refreshHoveringState();
}

void BattleFieldItem::spellChange()
{
    refreshHoveringState();
}

void BattleFieldItem::refreshCounters()
{
    for (BattleStackConstPtr stack : m_unitGraphics.keys()) {
        BattleStackSpriteItemPtr item          = m_unitGraphics[stack].spriteItem;
        m_unitGraphics[stack].animationEnabled = stack->count > 0 && stack->current.canDoAnything;
        if (stack->count <= 0) {
            item->setCounterVisible(false);
            continue;
        }
        item->setCounterVisible(true);
        auto mode = BattleStackSpriteItem::CounterMode::Normal;
        if (stack->current.hasBuff && stack->current.hasDebuff)
            mode = BattleStackSpriteItem::CounterMode::Mixed;
        else if (stack->current.hasBuff)
            mode = BattleStackSpriteItem::CounterMode::Buff;
        else if (stack->current.hasDebuff)
            mode = BattleStackSpriteItem::CounterMode::Debuff;
        item->setCounterMode(mode);
        item->setCounter(stack->count);
        const bool isOccupied = m_battleView.findStack(m_battleGeometry.neighbour(stack->pos.secondaryPos(), stack->pos.sightDirectionIsRight() ? BattleDirection::R : BattleDirection::L), true) != nullptr;
        item->setCounterCompact(isOccupied);
    }
}

void BattleFieldItem::refreshHoveringState()
{
    m_controlPlan.m_hoveredStack = nullptr;
    m_showAvailableForHovered    = false;

    auto battlePos = defaultGeometry.coordFromPos(m_mousePosition);
    m_hovered      = {};
    m_controlPlan.m_planMove.clear();
    m_controlPlan.m_planCast.clear();
    m_controlPlan.m_planMoveParams   = {};
    m_controlPlan.m_planAttackParams = {};

    if (m_battleGeometry.isValid(battlePos)) {
        m_hovered               = battlePos;
        const QPointF posInCell = m_mousePosition - defaultGeometry.hexCenterFromCoord(battlePos);
        hoverCell(battlePos, posInCell);
    }
    updateCursorsToPlan();
    updateUnitHighlights();

    m_controlPlan.sendPlanUpdated();

    this->update();
}

void BattleFieldItem::updateCursorsToPlan()
{
    QCursor cursor = m_cursorLibrary.getOther(ICursorLibrary::Type::Stop);
    if (m_controlPlan.m_planCastParams.isActive()) {
        QCursor castCur = m_cursorLibrary.getCast().value(0);
        cursor          = m_controlPlan.m_planCast.m_isValid ? castCur : m_cursorLibrary.getOther(ICursorLibrary::Type::Stop);

    } else if (m_controlPlan.m_planAttackParams.isActive() && m_controlPlan.m_planMove.isValid()) {
        if (m_controlPlan.m_planMove.m_attackMode == BattlePlanMove::Attack::Ranged)
            cursor = m_cursorLibrary.getOther(m_controlPlan.m_planMove.m_rangedAttackDenominator == 1 ? ICursorLibrary::Type::RangeAttack : ICursorLibrary::Type::RangeAttackBroken);
        else if (m_controlPlan.m_planMove.m_attackMode == BattlePlanMove::Attack::Melee)
            cursor = m_cursorLibrary.getAttackCursor(getCursorDirection(m_controlPlan.m_planAttackParams.m_attackDirection));
    } else if (m_controlPlan.m_planMoveParams.isActive() && m_controlPlan.m_planMove.isValid()) {
        cursor = m_cursorLibrary.getOther(m_controlPlan.m_selectedStack->library->traits.fly ? ICursorLibrary::Type::Fly : ICursorLibrary::Type::Walk);
    } else {
        if (m_controlPlan.m_hoveredStack)
            cursor = m_cursorLibrary.getOther(ICursorLibrary::Type::Question);
    }
    this->setCursor(cursor);
}

void BattleFieldItem::updateUnitHighlights()
{
    std::map<BattleStackConstPtr, int> hovered;
    hovered[m_controlPlan.m_hoveredStack] = 0;

    if (m_controlPlan.m_planCastParams.isActive()) {
        hovered.clear();
        for (auto& target : m_controlPlan.m_planCast.m_targeted) {
            if (target.stack) {
                int kills             = target.loss.deaths;
                hovered[target.stack] = kills;
            }
        }
    }
    if (m_controlPlan.m_planMove.isValid()) {
        for (auto& target : m_controlPlan.m_planMove.m_extraAffectedTargets) {
            if (target.stack) {
                int kills             = target.damage.avgRoll.loss.deaths;
                hovered[target.stack] = kills;
            }
        }
    }
    for (auto stack : m_unitGraphics.keys()) {
        UnitGraphics& graphics   = m_unitGraphics[stack];
        const bool    oldHovered = graphics.spriteItem->getHighlight(BattleStackSpriteItem::Highlight::Hovered);
        const bool    newHovered = hovered.count(stack) > 0;
        graphics.spriteItem->setHighlight(BattleStackSpriteItem::Highlight::Selected, (stack == m_controlPlan.m_selectedStack));
        graphics.spriteItem->setHighlight(BattleStackSpriteItem::Highlight::Hovered, newHovered);
        if (m_appSettings.battle().counterDamageHint)
            graphics.spriteItem->setCounterExtra(-hovered[stack]);

        graphics.sporadic.cfg.enabled = stack->isAlive() && m_controlAvailable; // @todo: not alive, but can move maybe.
        if (newHovered && !oldHovered && m_controlAvailable)
            graphics.sporadic.runNow();
    }
}

std::unique_ptr<AnimationSequencer> BattleFieldItem::makeSequencer()
{
    auto sequencer = std::make_unique<AnimationSequencer>(m_appSettings.battle(), m_musicBox);
    sequencer->enableSuperSpeed(m_superspeed);
    return sequencer;
}

void BattleFieldItem::addSpriteForBattleStack(BattleStackConstPtr stack)
{
    auto guiUnit = m_modelsProvider.units()->find(stack->library);

    auto sprite = guiUnit->getBattleSprite();
    Q_ASSERT(sprite);

    SpritePtr projectileSprite;
    if (!stack->library->presentationParams.spriteProjectile.empty()) {
        projectileSprite = guiUnit->getProjectileSprite();
        Q_ASSERT(projectileSprite);
    }

    BattleStackSpriteItem* item = new BattleStackSpriteItem(sprite, defaultGeometry.hexW1 * 2, m_unitGroup);
    auto                   seq  = makeSequencer();
    auto*                  h    = seq->addHandle(item, stack);
    h->changeAnim(BattleAnimation::StandStill);
    auto pos = stack->pos.mainPos();
    item->setZValue(zValueForPosAlive(pos));
    item->setIsLarge(stack->library->traits.large);
    item->setStartDirectionRight(stack->side == BattleStack::Side::Attacker);
    item->setAcceptedMouseButtons(Qt::MouseButtons());
    item->setAnimGroupSporadic(h->getAnimSettings(BattleAnimation::Nervous));

    m_unitGraphics[stack] = { item, projectileSprite, SporadicHandle{ { true, 4000, 4000, [item] { item->triggerSporadic(); }, [&]() -> bool { return m_sporadicOrchestrator.checkEventLimit(10000, 5); } } } };

    const QPointF posf = defaultGeometry.hexCenterFromExtCoord(stack->pos);
    item->setPos(posf);
}

}
