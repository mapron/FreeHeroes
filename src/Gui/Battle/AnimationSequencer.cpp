/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "AnimationSequencer.hpp"

#include "OneTimeCallAnimation.hpp"

#include "BattleField.hpp"

#include <QDebug>
#include <QDateTime>

namespace FreeHeroes::Gui {
using namespace Core;

namespace {
static const QMap<AnimationSequencer::BattleAnimation, AnimationSequencer::BattleSound> soundMapping {
    {AnimationSequencer::BattleAnimation::Move         , AnimationSequencer::BattleSound::Move},
    {AnimationSequencer::BattleAnimation::PainRanged   , AnimationSequencer::BattleSound::PainRanged},
    {AnimationSequencer::BattleAnimation::PainMelee    , AnimationSequencer::BattleSound::PainMelee},
    {AnimationSequencer::BattleAnimation::Death        , AnimationSequencer::BattleSound::Death},
    {AnimationSequencer::BattleAnimation::Turning      , AnimationSequencer::BattleSound::Move},
    {AnimationSequencer::BattleAnimation::MeleeUp      , AnimationSequencer::BattleSound::Melee},
    {AnimationSequencer::BattleAnimation::MeleeCenter  , AnimationSequencer::BattleSound::Melee},
    {AnimationSequencer::BattleAnimation::MeleeDown    , AnimationSequencer::BattleSound::Melee},
    {AnimationSequencer::BattleAnimation::RangedUp     , AnimationSequencer::BattleSound::Ranged},
    {AnimationSequencer::BattleAnimation::RangedCenter , AnimationSequencer::BattleSound::Ranged},
    {AnimationSequencer::BattleAnimation::RangedDown   , AnimationSequencer::BattleSound::Ranged},
    {AnimationSequencer::BattleAnimation::MoveStart    , AnimationSequencer::BattleSound::MoveStart},
    {AnimationSequencer::BattleAnimation::MoveFinish   , AnimationSequencer::BattleSound::MoveFinish},
    {AnimationSequencer::BattleAnimation::MagicUp      , AnimationSequencer::BattleSound::None}, // ?
    {AnimationSequencer::BattleAnimation::MagicCenter  , AnimationSequencer::BattleSound::None}, //
    {AnimationSequencer::BattleAnimation::MagicDown    , AnimationSequencer::BattleSound::None}, //
};
static const QMap<AnimationSequencer::BattleSound, std::string> soundResourceNames {
    {AnimationSequencer::BattleSound::Move         , "move"},
    {AnimationSequencer::BattleSound::PainRanged   , "wnce"},
    {AnimationSequencer::BattleSound::PainMelee    , "dfnd"},
    {AnimationSequencer::BattleSound::Death        , "kill"},
    {AnimationSequencer::BattleSound::Melee        , "attk"},
    {AnimationSequencer::BattleSound::Ranged       , "shot"},
    {AnimationSequencer::BattleSound::MoveStart    , "ext1"},
    {AnimationSequencer::BattleSound::MoveFinish   , "ext2"},
};
static const QMap<AnimationSequencer::BattleSound, int> soundDurationsMin {
    {AnimationSequencer::BattleSound::Move         , 200},
    {AnimationSequencer::BattleSound::PainRanged   , 200},
    {AnimationSequencer::BattleSound::PainMelee    , 200},
    {AnimationSequencer::BattleSound::Death        , 300},
    {AnimationSequencer::BattleSound::Melee        , 200},
    {AnimationSequencer::BattleSound::Ranged       , 200},
    {AnimationSequencer::BattleSound::MoveStart    , 200},
    {AnimationSequencer::BattleSound::MoveFinish   , 200},
};

}

AnimationSequencer::AnimationSequencerHandle::AnimationSequencerHandle(AnimationSequencer* parent, BattleStackSpriteItemPtr item, BattleStackConstPtr stack)
    : parent(parent), item(item), stack(stack){
    sight = item->getTmpDirectionRight() ? PosSight::ToRight : PosSight::ToLeft;
}

void AnimationSequencer::AnimationSequencerHandle::changeAnim(BattleAnimation type, bool reverse)
{
    const auto settings = this->getAnimSettings(type).setReverse(reverse);
    if (settings.durationMs > 0)
        item->setAnimGroup(settings);
}

void AnimationSequencer::AnimationSequencerHandle::playEffect(BattleAnimation type, int expectedDuration)
{
    if (stack->library->presentationParams.soundId.empty())
        return;

    AnimationSequencer::BattleSound soundType = soundMapping.value(type, AnimationSequencer::BattleSound::None);
    if (!stack->library->presentationParams.soundHasShoot && soundType == AnimationSequencer::BattleSound::Ranged)
        soundType = AnimationSequencer::BattleSound::Melee;
    if (!stack->library->presentationParams.soundHasMovementStart && (soundType == AnimationSequencer::BattleSound::MoveStart
                                                || soundType == AnimationSequencer::BattleSound::MoveFinish))
        soundType = AnimationSequencer::BattleSound::None;

    if (soundType == AnimationSequencer::BattleSound::None)
        return;

    if (expectedDuration < 0) {
        expectedDuration = this->getAnimDuration(type);
    }

    std::string resourceId = soundResourceNames.value(soundType);
    Q_ASSERT(!resourceId.empty());

    auto eff = Sound::IMusicBox::EffectSettings{stack->library->presentationParams.soundId + resourceId}
               .setExpectedDuration(std::max(expectedDuration, soundDurationsMin.value(soundType)))
               .setLoopAround(soundType == AnimationSequencer::BattleSound::Move)
               .setFadeIn(soundType == AnimationSequencer::BattleSound::Move ? 200 : 0)
               .setFadeOut(soundType == AnimationSequencer::BattleSound::Move ? 200 : 100);

    parent->m_musicBox.effectPrepare(eff)->play();
}



void AnimationSequencer::AnimationSequencerHandle::queueChangeAnim(BattleAnimation type, int pauseDuration)
{
    if (pauseDuration < 0)
        pauseDuration = getAnimDuration(type);

    parent->addCallback([this, type]{
        changeAnim(type);
    }, pauseDuration);
}

void AnimationSequencer::AnimationSequencerHandle::queuePlayEffect(AnimationSequencer::BattleAnimation type, int expectedDuration, int pauseDuration)
{
    if (expectedDuration <= 0)
        return;

    if (pauseDuration < 0)
        pauseDuration = getAnimDuration(type);

    parent->addCallback([this, type, expectedDuration]{
        playEffect(type, expectedDuration);
    }, pauseDuration);
}

bool AnimationSequencer::AnimationSequencerHandle::addOptionalAnim(BattleAnimation type) {
    if (!item->getSprite()->getFramesForGroup(static_cast<int>(type)))
        return false;
    queueChangeAnim(type);
    return true;
}

void AnimationSequencer::AnimationSequencerHandle::addPropertyAnimation(int msecs, const QByteArray& propertyName, const QVariant& endValue) {
    parent->addPropertyAnimation(item, msecs, propertyName, endValue);
}

void AnimationSequencer::AnimationSequencerHandle::addPosAnimation(int msecs, QPointF endValue) {
    addPropertyAnimation(msecs, "pos", endValue);
}

void AnimationSequencer::AnimationSequencerHandle::addPosTeleport(QPointF endValue)
{
    parent->addCallback([this, endValue]{
        item->setPos(endValue);
    }, 1);
}

void AnimationSequencer::AnimationSequencerHandle::addTurning(bool fullAnimation, bool toRight){
    const int duration = getAnimDuration(BattleAnimation::Turning);

    if (fullAnimation)
        queueChangeAnim(BattleAnimation::Turning);

    parent->addCallback([this, toRight]{
        item->setTempDirectionRight( toRight );
    }, 1);

    if (fullAnimation)
        parent->addCallback([this]{
            changeAnim(BattleAnimation::Turning, true);
        }, duration);
}

bool AnimationSequencer::AnimationSequencerHandle::addOptionalTurning(const BattlePosition from, const BattlePosition to, bool fullAnimation){
    const auto pseudoFrom = from.toDecartCoordinates();
    const auto pseudoTo   = to.toDecartCoordinates();
    if (pseudoFrom.x == pseudoTo.x)
        return false;

    return addOptionalTurning((pseudoFrom.x < pseudoTo.x) ? PosSight::ToRight : PosSight::ToLeft, fullAnimation);
}

bool AnimationSequencer::AnimationSequencerHandle::addOptionalTurning(PosSight currentSight, bool fullAnimation)
{
    if (currentSight == sight)
        return false;

    addTurning(fullAnimation, currentSight == PosSight::ToRight);

    sight = currentSight;
    return true;
}

bool AnimationSequencer::AnimationSequencerHandle::addOptionalTurningOrPause(PosSight currentSight, bool fullAnimation)
{
    const bool res = addOptionalTurning(currentSight, fullAnimation);
    if (!res)
        parent->pause(getAnimDuration(BattleAnimation::Turning));
    return res;
}

bool AnimationSequencer::AnimationSequencerHandle::addOptionalTurningToStart(bool full) {
    if (sight != stack->pos.sightDirection()) {
        addTurning(full, stack->pos.sightDirectionIsRight());
        queueChangeAnim(BattleAnimation::StandStill, 1);
        return true;
    }
    return false;
}

int AnimationSequencer::AnimationSequencerHandle::getAnimDuration(BattleAnimation type) const
{
    const int baseSpeedup = parent->getAnimSpeedupPercent(type);

    const auto & sprite = item->getSprite();
    const auto seq = sprite->getFramesForGroup(static_cast<int>(type));
    const int cycleDuration  = seq ? seq->params.animationCycleDuration : 1000;
    const int durationResult = cycleDuration * baseSpeedup / 100;
    const bool canBeIgnored = type == BattleAnimation::Turning
                              || type == BattleAnimation::MoveStart
                              || type == BattleAnimation::MoveFinish;
    if (canBeIgnored && durationResult < 20) // if less than 1 frame on 50fps, ignore that
        return 0;

    return std::max(durationResult, 1);
}

SpriteItem::AnimGroupSettings AnimationSequencer::AnimationSequencerHandle::getAnimSettings(BattleAnimation type) const
{
    SpriteItem::AnimGroupSettings settings = parent->getAnimSettings(type);
    settings.durationMs = getAnimDuration(type);
    return settings;
}

//// --------------------------  Sequencer -------------------------------

AnimationSequencer::AnimationSequencer(const Gui::IAppSettings::Battle & battleSettings, Sound::IMusicBox & musicBox)
    : m_battleSettings(battleSettings), m_musicBox(musicBox) {}

AnimationSequencer::AnimationSequencerHandle* AnimationSequencer::addHandle(BattleStackSpriteItemPtr item, BattleStackConstPtr stack) {
    m_handles << AnimationSequencerHandle{this, item, stack};
    return &m_handles.back();
}

void AnimationSequencer::addPropertyAnimation(QObject* target, int msecs, const QByteArray & propertyName, const QVariant & endValue)
{
    m_tree.addPropertyAnimation(target, msecs, propertyName, endValue);
}

void AnimationSequencer::addCallback(std::function<void ()> callback, int pauseDuration) {
    m_tree.addCallback(std::move(callback), pauseDuration);
}

void AnimationSequencer::playEffect(const std::string & soundId, int expectedDuration)
{
    auto eff = Sound::IMusicBox::EffectSettings{soundId}
               .setExpectedDuration(expectedDuration)
               .setLoopAround(false);

    m_musicBox.effectPrepare(eff)->play();
}
void AnimationSequencer::queuePlayEffect(const std::string & soundId, int expectedDuration, int pauseDuration)
{
    addCallback([this, soundId, expectedDuration]{
        playEffect(soundId, expectedDuration);
    }, pauseDuration);
}

int AnimationSequencer::getAnimSpeedupPercent(BattleAnimation type) const
{
    auto speedUpTime = [this](bool move){
        const auto t = (move ? m_battleSettings.walkTimePercent : m_battleSettings.otherTimePercent);
        const auto t2 = (m_superspeed) ? t * m_battleSettings.shiftSpeedPercent / 100 : t;
        return std::max(t2, 1);
    };
    switch(type) {
    case BattleAnimation::Move: return speedUpTime(true); break;
    case BattleAnimation::Nervous:  break;
    case BattleAnimation::StandStill:  break;
    case BattleAnimation::Turning: return speedUpTime(true); break;
    case BattleAnimation::MeleeUp:
    case BattleAnimation::MeleeCenter:
    case BattleAnimation::MeleeDown:
    case BattleAnimation::RangedUp:
    case BattleAnimation::RangedCenter:
    case BattleAnimation::RangedDown:
    case BattleAnimation::MagicUp:
    case BattleAnimation::MagicCenter:
    case BattleAnimation::MagicDown:
        return speedUpTime(false); break; // 400 just attack, 100 both attack and pain
    case BattleAnimation::PainMelee:
    case BattleAnimation::PainRanged:
        return speedUpTime(false); break;
    case BattleAnimation::MoveStart:
        return speedUpTime(true); break; // per frame
    case BattleAnimation::MoveFinish:
        return speedUpTime(true); break; // per frame
    case BattleAnimation::Death:
        return speedUpTime(false); break;
    default:
        break;
    }
    return 100;
}

SpriteItem::AnimGroupSettings AnimationSequencer::getAnimSettings(BattleAnimation type) const
{
    const bool useLoop =
            type == BattleAnimation::Move ||
            type == BattleAnimation::StandStill
            ;
    const int startFrame = type == BattleAnimation::Move ? 2 : 0;
    return SpriteItem::AnimGroupSettings(static_cast<int>(type), 1).setLoopOver(useLoop).setStartFrame(startFrame);
}

void AnimationSequencer::runSync(bool excludeInput) {
    m_tree.runSync(excludeInput);
}

void AnimationSequencer::beginParallel() {
    m_tree.beginParallel();
}

void AnimationSequencer::beginSequental() {
    m_tree.beginSequental();
}

void AnimationSequencer::endGroup() {
    m_tree.endGroup();
}

void AnimationSequencer::pause(int msec) {
    m_tree.pause(msec);
}

void AnimationSequencer::enableSuperSpeed(bool  superspeed)
{
    m_superspeed = superspeed;
}

}
