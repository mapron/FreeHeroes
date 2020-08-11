/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "AnimationTree.hpp"
#include "OneTimeCallAnimation.hpp"

#include <QPropertyAnimation>
#include <QPauseAnimation>
#include <QSequentialAnimationGroup>
#include <QParallelAnimationGroup>
#include <QEventLoop>

namespace FreeHeroes::Gui {

AnimationTree::AnimationTree()
    : m_root{std::make_unique<QSequentialAnimationGroup>()}
    , m_currentGroup{ m_root.get() }
{
}

AnimationTree::~AnimationTree()
{

}

void AnimationTree::addPropertyAnimation(QObject* target, int msecs, const QByteArray & propertyName, const QVariant & endValue)
{
    auto anim = new QPropertyAnimation(target, propertyName, m_currentGroup);
    anim->setDuration(msecs);
    anim->setEndValue(endValue);
    m_currentGroup->addAnimation(anim);
}

void AnimationTree::addCallback(std::function<void ()> callback, int pauseDuration) {
    auto anim = new OneTimeCallAnimation(callback, pauseDuration, m_currentGroup);
    m_currentGroup->addAnimation(anim);
}

void AnimationTree::runSync(bool excludeInput) {
    QEventLoop loop;
    QObject::connect(m_root.get(), &QAbstractAnimation::finished, &loop, &QEventLoop::quit);
    m_root->start();
    loop.exec(excludeInput ? QEventLoop::ExcludeUserInputEvents : QEventLoop::AllEvents);
}

void AnimationTree::runAsync()
{
    auto * root = m_root.release();
    root->start(QAbstractAnimation::DeleteWhenStopped);

    m_root = std::make_unique<QSequentialAnimationGroup>();
    m_currentGroup = m_root.get();
}

void AnimationTree::beginParallel() {
    QAnimationGroup * newGroup = new QParallelAnimationGroup(m_currentGroup);
    m_startedGroups << m_currentGroup;
    m_currentGroup = newGroup;
}

void AnimationTree::beginSequental() {
    QAnimationGroup * newGroup = new QSequentialAnimationGroup(m_currentGroup);
    m_startedGroups << m_currentGroup;
    m_currentGroup = newGroup;
}

void AnimationTree::endGroup() {
    m_currentGroup = m_startedGroups.takeLast();
}

void AnimationTree::pause(int msec) {
    if (msec <= 0)
        return;
    m_currentGroup->addAnimation(new QPauseAnimation(msec, m_currentGroup));
}


}
